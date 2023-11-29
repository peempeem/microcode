#pragma once

#include "pwm.h"

class LED
{
    public:
        LED(unsigned pin, bool inverted=false);

        void init();

		#ifdef PWM
        void usePWM(PWM& pwm);
		#endif
        void turnOn();
        void turnOff();
    
    private:
        unsigned pin;
        bool inverted;
		#ifdef PWM
        bool usingPWM = false;
		#endif

};
