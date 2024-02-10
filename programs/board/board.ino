#include "src/microcode/hal/hal.h"
#include "src/microcode/util/pvar.h"
#include "src/microcode/util/rate.h"
#include "src/microcode/wifi/wifilink.h"
#include "src/microcode/util/log.h"
#include "src/microcode/hardware/pwm.h"
#include "src/microcode/hardware/led.h"
#include <limits>
#include <VescUart.h>

struct WifiVar
{
    MAC mac;
    bool paired = false;
};

struct SpeedMode
{
    float accelMax;
    float breakMax;
    float speedMax;
    float reverseMax;
    float coastMax;

    SpeedMode(float accelMax, float breakMax, float speedMax, float reverseMax, float coastMax) :
        accelMax(accelMax), breakMax(breakMax), speedMax(speedMax), reverseMax(reverseMax), coastMax(coastMax)
    {

    }
};

struct VescValues1
{
    float batteryVoltage = 0;
    float rpm = 0;
};

struct VescValues2
{
    long  tachometerAbs = 0;
    float tempMosfet = 0;
    float tempMotor = 0;
    float ampHours = 0;
    float ampHoursCharged = 0;
    float wattHours = 0;
    float wattHoursCharged = 0;
};

PVar<WifiVar> wifiPVar("/mac.pvar");
MACLink maclink;

VescUart vesc;

PWM powerPWM(0, 10000, 16);
PWM linkPWM(1, 10000, 16);
PWM vescPWM(2, 10000, 16);

LED leftLED(33);
LED middleLED(25);
LED rightLED(26);

Rate powerBlinkRate(1);
Rate linkBlinkRate(1);
Rate vescBlinkRate(1);
bool vescConnected = false;

Rate speedChangeRate(100);

float controlValue = -1;
float speed = 0;

SpeedMode speedMode(0.15f, 0.20f, 0.2f, -0.05f, 0.10f);

MessageStream vv1Stream(11, sizeof(VescValues1), 20);
MessageStream vv2Stream(12, sizeof(VescValues2), 4);

void pollVesc(void* data)
{
    while (true)
    {
        vescConnected = vesc.getVescValues();
        if (vescConnected)
        {
            vesc.setDuty(speed, 55);
            vesc.setDuty(speed, 56);
            
            vv1Stream.lock();
            VescValues1* vv1 = (VescValues1*) vv1Stream.get();
            vv1->batteryVoltage = vesc.data.inpVoltage;
            vv1->rpm = vesc.data.rpm;
            vv1Stream.unlock();

            vv2Stream.lock();
            VescValues2* vv2 = (VescValues2*) vv2Stream.get();
            vv2->tachometerAbs = vesc.data.tachometerAbs;
            vv2->tempMosfet = vesc.data.tempMosfet;
            vv2->tempMotor = vesc.data.tempMotor;
            vv2->ampHours = vesc.data.ampHours;
            vv2->ampHoursCharged = vesc.data.ampHoursCharged;
            vv2->wattHours = vesc.data.wattHoursCharged;
            vv2->wattHoursCharged = vesc.data.wattHoursCharged;
            vv2Stream.unlock();
        }
    }
}

void updateSpeed(void* data)
{
    uint32_t last = millis();
    while (true)
    {
        float dt = (millis() - last) / 1000.0f;
        last = millis();
        if (controlValue >= 0)
        {
            float cval = (controlValue - 0.5f) * 2;
            float accel;

            if (cval > 0.1f)
            {
                if (speed > 0)
                    accel = speedMode.accelMax;
                else
                    accel = speedMode.breakMax;
                cval -= 0.1f;
            } 
            else if (cval < -0.1f)
            {
                if (speed > 0)
                    accel = speedMode.breakMax;
                else
                    accel = speedMode.accelMax;
                cval += 0.1f;
            } 
            else
                accel = 0;
            
            cval /= 0.9f;
            float newSpeed = speed + accel * cval * dt;
            
            if (newSpeed > speedMode.speedMax && cval > 0)
            {
                speed -= speedMode.coastMax * dt;
                if (speed < speedMode.speedMax)
                    speed = speedMode.speedMax;
            }
            else if (newSpeed < speedMode.reverseMax && cval < 0)
            {
                speed += speedMode.coastMax * dt;
                if (speed > speedMode.reverseMax)
                    speed = speedMode.reverseMax;
            }
            else
                speed = newSpeed;
        }
        else 
        {
            if (speed > 0)
            {
                speed -= speedMode.coastMax * dt;
                if (speed < 0)
                    speed = 0;
            }
            else if (speed < 0)
            {
                speed += speedMode.coastMax * dt;
                if (speed > 0)
                    speed = 0;
            }
        }
        speedChangeRate.sleep();
    }
}

void setup()
{
    Serial.begin(115200);
    sysAttachDebug(&Serial);
    filesys.init();
    wifilink.init("Board");

    powerPWM.init();
    linkPWM.init();
    vescPWM.init();
    leftLED.init();
    middleLED.init();
    rightLED.init();

    leftLED.usePWM(powerPWM);
    middleLED.usePWM(linkPWM);
    rightLED.usePWM(vescPWM);

    *((VescValues1*) vv1Stream.get()) = VescValues1();
    *((VescValues2*) vv1Stream.get()) = VescValues2();

    if (wifiPVar.load() && wifiPVar.data.paired)
    {
        maclink = MACLink(wifiPVar.data.mac);
        maclink.registerMessageStream(vv1Stream);
        //maclink.registerMessageStream(vv2Stream);
        wifilink.add(maclink);
    }

    wifilink.setPairing(!wifiPVar.data.paired);

    Serial2.begin(115200, SERIAL_8N1, 13, 15);
    vesc.setSerialPort(&Serial2);

    xTaskCreatePinnedToCore(pollVesc, "vesc_task", 4096, NULL, 1, NULL, 1);
    xTaskCreate(updateSpeed, "speed_task", 1024, NULL, 2, NULL);
}

void loop()
{
    if (!wifiPVar.data.paired)
    {
        std::list<MAC> pairings = wifilink.getPairings();
        if (pairings.size() > 0)
        {
            maclink = MACLink(pairings.front());
            maclink.registerMessageStream(vv1Stream);
            maclink.registerMessageStream(vv2Stream);
            wifilink.add(maclink);
            wifiPVar.data.mac = maclink.getMAC();
            wifiPVar.data.paired = true;
            wifiPVar.save();
            wifilink.setPairing(false);
        }
    }

    if (maclink.connected())
    {
        while (!maclink.messages.empty())
        {
            Message& msg = maclink.messages.front();

            switch (msg.type())
            {
                case 9:
                {
                    speedMode = *((SpeedMode*) msg.data());
                    break;
                }

                case 10:
                {
                    controlValue = *((float*) msg.data());
                    break;
                }
            }
            
            linkBlinkRate.setHertz(maclink.rssi() / 15.0f, true);
            maclink.messages.pop();
        }

        linkPWM.write(1 - linkBlinkRate.getStageCos());
    }
    else
    {
        controlValue = -1;
        linkPWM.write(0);
    }

    powerPWM.write(1 - powerBlinkRate.getStageCos());
    vescPWM.write(vescConnected ? 1 - vescBlinkRate.getStageCos() : 0);

    wifilink.update();
}
