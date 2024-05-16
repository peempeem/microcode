#pragma once

#include <stdint.h>

class Rate 
{
    public:
        Rate(float hertz=0);

        void setMs(unsigned ms, bool keepStage=false);
        void setHertz(float hertz, bool keepStage=false);
        
        void reset();
        bool isReady();
        void sleep();

        float getStage();
        float getStageSin(float offset=0);
        float getStageCos(float offset=0);

    private:
        uint64_t inverseRate;
        uint64_t start;
        uint64_t last;
};
