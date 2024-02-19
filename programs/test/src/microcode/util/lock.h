#pragma once

#include "../hal/hal.h"

class SpinLock
{
    public:
        SpinLock() {}
        void lock() { portENTER_CRITICAL_SAFE(&_spinlock); }
        void unlock() { portEXIT_CRITICAL_SAFE(&_spinlock); }
    
    private:
        portMUX_TYPE _spinlock = portMUX_INITIALIZER_UNLOCKED;
};

class BinarySemaphore
{
    public:
        BinarySemaphore() { vSemaphoreCreateBinary(_semaphore); }
        ~BinarySemaphore() { vSemaphoreDelete(_semaphore); }

        bool lock(TickType_t timeout=portMAX_DELAY) { return xSemaphoreTake(_semaphore, timeout) == pdTRUE; }
        bool unlock() { return xSemaphoreGive(_semaphore) == pdTRUE; }
    
    private:
        xSemaphoreHandle _semaphore;
};
