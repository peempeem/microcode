#pragma once

#include "../util/rate.h"

#if defined sysConfigurePWM && defined sysAttachPin && defined sysDetachPin && defined sysPWMrite

class PWM
{
    public:
        PWM();
        PWM(unsigned channel, unsigned frequency, unsigned resolution);

        void init();

        void write(float duty);

        unsigned channel();
        unsigned frequency();
        unsigned resolution();
    
    private:
        bool defaultConstructor;
        unsigned _channel;
        unsigned _frequency;
        unsigned _resolution;
        Rate ramp;
};

#endif
