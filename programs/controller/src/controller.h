#pragma once

#include "microcode/hal/hal.h"
#include "microcode/util/rate.h"
#include "microcode/hardware/button.h"
#include "microcode/espwifi/wifilink.h"

enum PowerState
{
    discharging,
    charging,
    powerSaving,
    invalid
};

class Controller
{
    public:
        Rate sampleRate = Rate(100);
        Rate batterySampleRate = Rate(10);

        float powerSavingLevel = 0.15f;

        Button mainButton;
        Button leftButton;
        Button rightButton;

        Controller();

        void init();
        void update(bool sleep=false);

        float wheelValue();
        float batteryVoltage();
        float batteryLevel();
        unsigned powerState();

    private:
        float _wheelValue = 0.0f;
        float _batteryVoltage = 0.0f;
        float _batteryLevel = 4.2f;
        unsigned _powerState = PowerState::discharging;
} static controller;
