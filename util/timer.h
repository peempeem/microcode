#pragma once

#include "rate.h"

class Timer
{
    public:
        Timer(unsigned ms=0);

        void set(unsigned ms);
        void reset();
        void ring();
        void silence();

        bool isRinging();

        float getStage();
        float getStageSin();
        float getStageCos();
    
    private:
        unsigned ms;
        unsigned start;
        unsigned end;
        bool silenced;
};