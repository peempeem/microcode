#include "controller.h"

enum Pin
{
    chipIn = 34,
    power = 14,
    battery = 39,
    wheel = 37,
    mainButton = 13,
    leftButton = 35,
    rightButton = 0
};

Controller::Controller()
{
    mainButton = Button(Pin::mainButton, true, false);
    leftButton = Button(Pin::leftButton);
    rightButton = Button(Pin::rightButton);
}

void Controller::init()
{
    Serial.begin(115200);

    pinMode(Pin::power, INPUT);
    pinMode(Pin::battery, INPUT);
    pinMode(Pin::wheel, INPUT);

    mainButton.init();
    leftButton.init();
    rightButton.init();

    wifilink.init("Controller");
    wifilink.setClockSpeed(240);
}

void Controller::update(bool sleep)
{
    if (sleep || sampleRate.isReady())
    {
        mainButton.update();
        leftButton.update();
        rightButton.update();

        _wheelValue = 1 - sysAnalogRead(Pin::wheel) / 4096.0f;

        if (batterySampleRate.isReady())
        {
            _batteryVoltage = 133 * 3.3f * sysAnalogRead(Pin::battery) / (100 * 4096.0f);
            
            float newBatteryLevel;
            if (_batteryVoltage >= 3.8f)
                newBatteryLevel = 1;
            else if (_batteryVoltage <= 3.3f)
                newBatteryLevel = 0;
            else
                newBatteryLevel = (1 - cos(PI * (_batteryVoltage - 3.3f) / 0.5f)) / 2.0f;
            _batteryLevel = _batteryLevel * 0.95f + newBatteryLevel * 0.05f;

            unsigned newPowerState;
            if (digitalRead(Pin::power))
                newPowerState = PowerState::charging;
            else
            {
                if (_batteryLevel > powerSavingLevel)
                    newPowerState = PowerState::discharging;
                else
                    newPowerState = PowerState::powerSaving;
            }

            if (newPowerState != _powerState)
            {
                switch (newPowerState)
                {
                    case PowerState::discharging:
                        if (_powerState == PowerState::charging)
                            wifilink.setClockSpeed(240);
                        break;
                    
                    case PowerState::charging:
                        wifilink.setClockSpeed(80);
                        break;
                    
                    case PowerState::powerSaving:    
                        break;
                }
                _powerState = newPowerState;
            }
        }
    }
    if (sleep)
        sampleRate.sleep();
}

float Controller::wheelValue()
{
    return _wheelValue;
}

float Controller::batteryVoltage()
{
    return _batteryVoltage;
}

float Controller::batteryLevel()
{
    return _batteryLevel;
}

unsigned Controller::powerState()
{
    return _powerState;
}
