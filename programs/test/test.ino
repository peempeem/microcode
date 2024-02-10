#include "src/microcode/hal/hal.h"
#include "src/microcode/hardware/suart.h"
#include "src/microcode/util/log.h"
#include "src/microcode/MCP23S08/mcp23s08.h"
#include "src/microcode/ADS1120/ads1120.h"
#include "src/microcode/util/rate.h"
#include "src/microcode/grid/msg.h"
#include "src/microcode/grid/hub.h"
#include "src/microcode/usb/usbcom.h"
#include "src/microcode/util/filesys.h"
#include "src/microcode/ota/ota.h"
#include "src/microcode/util/pvar.h"

#include <Adafruit_NeoPixel.h>
#include <Adafruit_BNO08x.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPmDNS.h>


#define MCP_CS  22
#define MCP_RST 27

#define ADC_CS  15
#define ADC_RST 39

#define IMU_CS  5
#define IMU_INT 25
#define IMU_RST 14

#define PXL_PIN 21
#define NUM_PXL 4

#define RX0 34
#define TX0 33
#define RX1 35
#define TX1 32

uint8_t driverMap[] = { 1, 0, 3, 2, 5, 4, 7, 6 };

PACK(struct StageState
{
    uint64_t timestamp;
    union Initialization
    {
        struct Fields
        {
            uint8_t filesys : 1;
            uint8_t wifi    : 1;
            uint8_t mcp     : 1;
            uint8_t adc     : 1;
            uint8_t imu     : 1;
            uint8_t comm0   : 1;
            uint8_t comm1   : 1;
            uint8_t debug   : 1;
        } single;
        uint8_t all;
    } init;

    union Drivers
    {
        struct Fields
        {
            uint8_t s0 : 1;
            uint8_t s1 : 1;
            uint8_t s2 : 1;
            uint8_t s3 : 1;
            uint8_t s4 : 1;
            uint8_t s5 : 1;
            uint8_t m0 : 1;
            uint8_t m1 : 1;
        } single;
        uint8_t all;
    } drivers;
    
    float pressure[4];
    float ypr[3];
    float power;
}) ss;

PACK(struct SetDevice
{
    uint8_t driver;
    uint8_t on;
});

SPIClass spiBus(VSPI);
MCP23S08 mcp(&spiBus, MCP_CS, MCP_RST);
ADS1120 adc(&spiBus, ADC_CS, ADC_RST);
Adafruit_BNO08x imu(IMU_RST);
Adafruit_NeoPixel pixel(NUM_PXL, PXL_PIN, NEO_GRB + NEO_KHZ800);
USBMessageBroker usb;

Rate r(1);
Rate g(2);
Rate b(3);

Rate pixelSend(100);
Rate sendStateRate(100);

unsigned readPin = 0;

struct euler_t 
{
  float yaw;
  float pitch;
  float roll;
} ypr;

sh2_SensorValue_t sensorValue;

struct WiFiData
{
    char ssid[64];
    char host[64];
    char pass[64];
};

PVar<WiFiData> wifiData("/wifi");

void setup()
{
    Log("init") << "Begin initialization...";

    suart0.init(500000);
    ss.init.single.debug = 1;
    Log() >> *suart0.getTXStream();
    usb.attachStreams(suart0.getTXStream(), suart0.getRXStream());

    if (!filesys.init())
        Log("init") << "File System -> [ FAILED ]";
    else
    {
        ss.init.single.filesys = 1;
        Log("init") << "File System -> [ SUCCESS ]";
    }

    wifiData.load();
    
    spiBus.begin();

    if (!mcp.begin())
        Log("init") << "MCP GPIO Extender -> [ FAILED ]";
    else
    {
        for (unsigned i = 0; i < 8; i++)
            mcp.setMode(i, OUTPUT);
        ss.init.single.mcp = 1;
        Log("init") << "MCP GPIO Extender -> [ SUCCESS ]";
    }

    if (!adc.begin())
        Log("init") << "ADS1120 ADC -> [ FAILED ]";
    else
    {
        adc.setGain(ADS1120::Gain1);
        adc.setDataRate(ADS1120::T2000);
        adc.setVoltageRef(ADS1120::External_REFP0_REFN0); 
        adc.setMux(ADS1120::AIN0_AVSS);
        adc.setConversionMode(ADS1120::Continuous);
        ss.init.single.adc = 1;
        Log("init") << "ADS1120 ADC -> [ SUCCESS ]";
    }

    if (!imu.begin_SPI(IMU_CS, IMU_INT, &spiBus))
        Log("init") << "BNO085 IMU -> [ FAILED ]";
    else
    {
        ss.init.single.imu = 1;
        Log("init") << "BNO085 IMU -> [ SUCCESS ]";
    }

    Log("init") << "Initialization complete";

    if (ss.init.single.filesys)
        filesys.logMap(filesys.map("/"));

    //Serial1.begin(500000, SERIAL_8N1, RX1, TX1);
    //usb.begin(500000, RX1, TX1);

    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(wifiData.data.ssid, wifiData.data.pass);
    MDNS.begin(wifiData.data.host);
    MDNS.addService("http", "tcp", ota.init());
}

void quaternionToEuler(float qr, float qi, float qj, float qk, euler_t* ypr, bool degrees=false)
{
    float sqr = sq(qr);
    float sqi = sq(qi);
    float sqj = sq(qj);
    float sqk = sq(qk);

    ypr->yaw = atan2(2.0 * (qi * qj + qk * qr), (sqi - sqj - sqk + sqr));
    ypr->pitch = asin(-2.0 * (qi * qk - qj * qr) / (sqi + sqj + sqk + sqr));
    ypr->roll = atan2(2.0 * (qj * qk + qi * qr), (-sqi - sqj + sqk + sqr));

    if (degrees) 
    {
        ypr->yaw *= RAD_TO_DEG;
        ypr->pitch *= RAD_TO_DEG;
        ypr->roll *= RAD_TO_DEG;
    }
}

void loop()
{
    if (ss.init.single.imu)
    {
        if (imu.wasReset())
            imu.enableReport(SH2_ARVR_STABILIZED_RV, 5000);
        
        if (imu.getSensorEvent(&sensorValue))
        {
            switch (sensorValue.sensorId) 
            {
                case SH2_ARVR_STABILIZED_RV:
                {
                    quaternionToEuler(
                        sensorValue.un.arvrStabilizedRV.real, 
                        sensorValue.un.arvrStabilizedRV.i,
                        sensorValue.un.arvrStabilizedRV.j,
                        sensorValue.un.arvrStabilizedRV.k,
                        &ypr, 
                        true);
                }
            }
        }
    }

    if (pixelSend.isReady())
    {
        for (unsigned i = 0; i < 4; i++)
        {
            pixel.setPixelColor(i, 
                pixel.Color(
                    r.getStageCos(0.1f * i) * 255, 
                    g.getStageCos(0.1f * i) * 255, 
                    b.getStageCos(0.1f * i) * 255));
        }
        pixel.show();
    }

    ss.pressure[readPin++] = 20 + 230 * ((adc.readVoltage(5) - 0.204f) / 4.692f);
    readPin %= 4;
    adc.setMux(readPin + ADS1120::AIN0_AVSS);

    while (usb.messages.size())
    {
        USBMessageBroker::Message& msg = usb.messages.top();
        switch (msg.type())
        {
            case 1:
            {
                SetDevice* setdevcmd = (SetDevice*) msg.data();
                if (setdevcmd->on)
                    ss.drivers.all |= 1 << setdevcmd->driver;
                else
                    ss.drivers.all &= ~(1 << setdevcmd->driver);
                mcp.write(driverMap[setdevcmd->driver], setdevcmd->on);
                break;
            }
        }
        usb.messages.pop();
    }

    if (sendStateRate.isReady())
    {
        ss.timestamp = sysTime();

        ss.power = 12 * 0.075;
        for (unsigned i = 0; i < 8; i++)
        {
            if (0x01 & (ss.drivers.all >> i))
                ss.power += 12 * 0.2;
        }

        ss.ypr[0] = ypr.yaw;
        ss.ypr[1] = ypr.pitch;
        ss.ypr[2] = ypr.roll;

        //usb.send(0, 0, (uint8_t*) &ss, sizeof(ss));
    }

    //usb.update();


    
    // if (debug.isReady())
    // {
    //     if (ss.init.single.adc || ss.init.single.imu)
    //         logc("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            
    //     if (ss.init.single.adc)
    //     {
    //         log("ADC", "Voltage: ", false);
    //         logc(adc.readVoltage());
    //     }

    //     if (ss.init.single.imu)
    //     {
    //         log("IMU", "Yaw: ", false);
    //         logc(ypr.yaw, false);
    //         logc("\tPitch: ", false);
    //         logc(ypr.pitch, false);
    //         logc("\tRoll: ", false);
    //         logc(ypr.roll);
    //     }
    // }

    // while (comm.available())
    //     comm.write(comm.read());
    
    // while (comm.available())
    // {
    //     trans.insert(comm.read());
    //     if (trans.packets.size())
    //     {
    //         log("recv", sysMicros() - start, false);
    //         logc('\t', false);
    //         logc((char*) trans.packets.front().get().data);
    //         trans.packets.pop();
    //     }
    // }   
}
