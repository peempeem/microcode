#include "src/microcode/hal/hal.h"
#include "src/microcode/util/pvar.h"
#include "src/microcode/util/eventScheduler.h"
#include "src/microcode/graphics/gui/gui.h"
#include "src/microcode/util/log.h"
#include "src/controller.h"
#include "src/screens.h"

struct WiFiVar
{
    MAC mac;
    bool paired = false;
};

PVar<WiFiVar> wifiPVar("/mac.pvar");
MACLink maclink;

GUI gui;
BlackScreen* blackScreen;
BootScreen* bootScreen;
ConnectScreen* connectScreen;
HomeScreen* homeScreen;

BatteryIcon* battery;
WiFiIcon* wifi;

EventScheduler scheduler;

enum Events
{
    bootScreen_transitionToConnectScreen,
    connectScreen_transitionToHomeScreen,
    connectScreen_connectionLossRecover,
    unpair_timeout
};

unsigned lastPowerState;

MessageStream controlValuesStream(10, sizeof(float), 50);

void pollController(void* data)
{
    while (true)
        controller.update(true);
}

void setup()
{
    controller.init();
    filesys.init();
    gui.init();

    if (wifiPVar.load() && wifiPVar.data.paired)
    {
        maclink = MACLink(wifiPVar.data.mac);
        maclink.registerMessageStream(controlValuesStream);
        wifilink.add(maclink);
    }

    battery = new BatteryIcon();
    wifi = new WiFiIcon();
    gui.notifications.addIcon(battery);
    gui.notifications.addIcon(wifi);

    blackScreen = new BlackScreen();
    bootScreen = new BootScreen();
    connectScreen = new ConnectScreen();
    homeScreen = new HomeScreen();

    gui.addScreen(blackScreen);
    gui.addScreen(bootScreen);
    gui.addScreen(connectScreen);
    gui.addScreen(homeScreen);
    gui.setMainScreen(blackScreen);
    gui.transitionTo(bootScreen, Transitions::slideRight, 500);
    scheduler.schedule(Events::bootScreen_transitionToConnectScreen, 1500);

    lastPowerState = PowerState::invalid;
    xTaskCreate(pollController, "pollController", 1024, NULL, 3, NULL);
}

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

SpeedMode speedModes[] =
{
    SpeedMode(0.15f, 0.17f,   0.2f,   -0.05f, 0.05f),
    SpeedMode(0.17f, 0.20f,   0.5f,   -0.07f, 0.06f),
    SpeedMode(0.19f, 0.23f,   0.8f,   -0.10f, 0.07f),
    SpeedMode(0.21f, 0.26f,   1.0f,   -0.15f, 0.08f)
};

unsigned speedMode = 0;
unsigned speedModeMax = 4;

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

void loop()
{
    if (controller.powerState() != lastPowerState)
    {
        switch (controller.powerState())
        {
            case PowerState::discharging:
            {
                battery->stopBlinking();
                gui.refereshRate.setHertz(60, true);
                break;
            }

            case PowerState::charging:
            {
                battery->startBlinking();
                gui.refereshRate.setHertz(15, true);
                break;
            }

            case PowerState::powerSaving:
            {
                battery->startBlinking();
                gui.refereshRate.setHertz(15, true);
                break;
            }
        }
        lastPowerState = controller.powerState();
    }

    if (controller.batteryLevel() >= 0.8f)
        battery->showIcon(battery->full);
    else if (controller.batteryLevel() >= 0.6f)
        battery->showIcon(battery->percent75);
    else if (controller.batteryLevel() >= 0.4f)
        battery->showIcon(battery->percent50);
    else if (controller.batteryLevel() >= 0.2)
        battery->showIcon(battery->percent25);
    else
        battery->showIcon(battery->empty);
    
    if (maclink.rssi() > 30)
        wifi->showIcon(wifi->wifiFull);
    else if (maclink.rssi() > 15)
        wifi->showIcon(wifi->wifi3);
    else if (maclink.rssi() > 0)
        wifi->showIcon(wifi->wifi2);
    else if (maclink.rssi() > -100)
        wifi->showIcon(wifi->wifi2);
    else
        wifi->showIcon(wifi->wifiNone);

    while (!scheduler.currentEvents.empty())
    {
        switch (scheduler.currentEvents.front())
        {
            case Events::bootScreen_transitionToConnectScreen:
            {
                if (gui.isMainScreen(bootScreen))
                {
                    gui.transitionTo(connectScreen, Transitions::slideRight, 500);
                    if (wifiPVar.data.paired)
                    {
                        connectScreen->wifi.startCycling();
                        connectScreen->wifi.stopBlinking();
                    }
                    else
                    {
                        connectScreen->wifi.showIcon(connectScreen->wifi.pairing);
                        connectScreen->wifi.startBlinking();
                    }
                }
                break;
            }

            case Events::connectScreen_transitionToHomeScreen:
            {
                if (gui.isMainScreen(connectScreen))
                    gui.transitionTo(homeScreen, Transitions::slideRight, 500);
                Message msg(9, (uint8_t*) &speedModes[speedMode], sizeof(SpeedMode));
                maclink.send(msg);
                break;
            }

            case Events::connectScreen_connectionLossRecover:
            {
                if (gui.isMainScreen(connectScreen) && !maclink.connected())
                {
                    connectScreen->wifi.stopBlinking();
                    connectScreen->wifi.startCycling();
                }
                break;
            }
        }
        scheduler.currentEvents.pop();
    }

    if (gui.isMainScreen(connectScreen))
    {
        if (!wifiPVar.data.paired)
        {
            connectScreen->text = "Pairing";
            if (wifilink.getScans().empty())
            {
                if (!wifilink.isScanning())
                    wifilink.scan("Board");
            }
            else
            {
                maclink = MACLink(wifilink.getScans().front());
                maclink.registerMessageStream(controlValuesStream);
                wifilink.add(maclink);
                wifiPVar.data.mac = maclink.getMAC();
                wifiPVar.data.paired = true;
                wifiPVar.save();
                wifilink.emptyScans();
            }
        }
        else
        {
            if (!scheduler.has(connectScreen_transitionToHomeScreen) && maclink.connected())
            {
                connectScreen->wifi.showIcon(connectScreen->wifi.wifiFull);
                connectScreen->wifi.startBlinking();
                connectScreen->wifi.stopCycling();
                scheduler.schedule(connectScreen_transitionToHomeScreen, 1000);
            }

            if (maclink.connected())
                connectScreen->text = "AOS";
            else if (!scheduler.has(connectScreen_connectionLossRecover))
                connectScreen->text = "Paired";
        }
    }
    else if (gui.isMainScreen(homeScreen))
    {
        if (!maclink.connected())
        {
            gui.transitionTo(connectScreen, Transitions::slideLeft, 500);
            connectScreen->text = "LOS";
            connectScreen->wifi.showIcon(connectScreen->wifi.wifiNone);
            connectScreen->wifi.startBlinking();
            connectScreen->wifi.stopCycling();
            connectScreen->wifi.wifi1.allocate();
            scheduler.schedule(connectScreen_connectionLossRecover, 1500);
        }
        else
        {
            unsigned lTaps = controller.leftButton.getTapEvent().taps;
            unsigned rTaps = controller.rightButton.getTapEvent().taps;
            if (lTaps || rTaps)
            {
                if (lTaps)
                {
                    if (!speedMode && lTaps > 1)
                        speedMode = speedModeMax - 1;
                    else
                    {
                        speedMode -= lTaps;
                        if (speedMode >= speedModeMax)
                            speedMode = 0;
                    }
                }
                
                if (rTaps)
                {
                    if (speedMode == speedModeMax - 1 && rTaps > 1)
                        speedMode = 0;
                    else
                    {
                        speedMode += rTaps;
                        if (speedMode >= speedModeMax)
                            speedMode = speedModeMax - 1;
                    }
                }

                Message msg(9, (uint8_t*) &speedModes[speedMode], sizeof(SpeedMode));
                maclink.send(msg);
            }

            if (controller.mainButton.releasedElapsedTime() < 250)
            {
                float throttle = controller.wheelValue();
                homeScreen->throttle.enable = true;
                homeScreen->throttle.power = throttle;

                controlValuesStream.lock();
                *((float*) controlValuesStream.get()) = throttle;
                controlValuesStream.unlock();
            }
            else
            {
                homeScreen->throttle.enable = false;

                controlValuesStream.lock();
                *((float*) controlValuesStream.get()) = -1;
                controlValuesStream.unlock();
            }

            homeScreen->setSpeedMode(speedMode);
            //homeScreen->setMemoryUsage(controller.batteryVoltage() * 100);
            homeScreen->setMemoryUsage(sysMemUsage() * 100);
        }
    }

    if (wifiPVar.data.paired && 
        controller.mainButton.releasedElapsedTime() > 4000 && 
        controller.leftButton.pressedElapsedTime() > 4000 && 
        controller.rightButton.pressedElapsedTime() > 4000 &&
        !scheduler.has(unpair_timeout))
    {
        wifiPVar.data.paired = false;
        wifiPVar.save();
        wifilink.remove(maclink);
        connectScreen->wifi.showIcon(connectScreen->wifi.pairing);
        connectScreen->wifi.startBlinking();
        gui.transitionTo(connectScreen, Transitions::slideLeft, 500);
        scheduler.schedule(unpair_timeout, 2000);
    }

    while (!maclink.messages.empty())
    {
        Message& msg = maclink.messages.front();
        switch (msg.type())
        {
            case 11:
            {
                VescValues1* values = (VescValues1*) msg.data();
                homeScreen->boardBattery.setBattery((values->batteryVoltage - 31) / 11.0f);
                homeScreen->setSpeed(values->rpm *  (16 / 36.0f) * 100 * 2 * PI * 0.0393701f * 0.000015782828283f * 60 / 14.0f);                
                break;
            }

            case 12:
            {
                VescValues2* values = (VescValues2*) msg.data();
                Log("tempMosfet") << values->tempMosfet;
                Log("tempMotor") << values->tempMotor;
                Log("ampHours") << values->ampHours;
                break;
            }
        }
        maclink.messages.pop();
    }
    
    scheduler.update();
    wifilink.update();
    gui.update();
}
