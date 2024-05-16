#pragma once

#include "pwm.h"

class LED
{
    public:
        LED(unsigned pin, bool inverted=false);

        void init();

        void usePWM(PWM& pwm);
        void turnOn();
        void turnOff();
    
    private:
        unsigned pin;
        bool inverted;
        bool usingPWM = false;
};
