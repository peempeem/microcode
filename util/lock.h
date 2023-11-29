#pragma once

#include "../hal/hal.h"

class Lock
{
    public:
        Lock();

        void lock();
        void unlock();
    
    private:
        #if __has_include(<esp.h>)
        portMUX_TYPE spinlock;
        #endif
};
