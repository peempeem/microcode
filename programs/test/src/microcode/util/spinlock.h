#pragma once

#include "../hal/hal.h"

class SpinLock
{
    public:
        bool lock(BaseType_t waitCycles=portMUX_NO_TIMEOUT);
        void unlock();
    
    private:
        portMUX_TYPE _spinlock = portMUX_INITIALIZER_UNLOCKED;
};
