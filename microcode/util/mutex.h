#pragma once

#include "../hal/hal.h"
#include "sharedbuf.h"

class Mutex
{
    public:
        bool lock(TickType_t timeout=portMAX_DELAY);
        bool unlock();
    
    private:
        SharedBuffer buf;
        SemaphoreHandle_t _semaphore = NULL;
};
