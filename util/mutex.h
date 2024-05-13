#pragma once

#include "../hal/hal.h"
#include "sharedbuf.h"

class Mutex
{
    public:
        Mutex(int maxCount=-1);
        bool lock(TickType_t timeout=portMAX_DELAY);
        bool unlock();
    
    private:
        SharedBuffer _buf;
        SemaphoreHandle_t _semaphore;
};
