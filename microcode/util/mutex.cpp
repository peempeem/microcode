#include "mutex.h"

//
//// Mutex Class
//

Mutex::Mutex(int maxCount)
{
    _buf = SharedBuffer(sizeof(StaticSemaphore_t));
        
    if (maxCount == -1)
        _semaphore = xSemaphoreCreateMutexStatic((StaticSemaphore_t*) _buf.data());
    else
        _semaphore = xSemaphoreCreateCountingStatic(maxCount, 0, (StaticSemaphore_t*) _buf.data());
}

bool Mutex::lock(TickType_t timeout)
{    
    return xSemaphoreTake(_semaphore, timeout) == pdTRUE;
}

bool Mutex::unlock()
{
    return xSemaphoreGive(_semaphore) == pdTRUE;
}
