#include "mutex.h"

//
//// Mutex Class
//

bool Mutex::lock(TickType_t timeout)
{    
    if (!_semaphore)
    {
        buf = SharedBuffer(sizeof(StaticSemaphore_t));
        _semaphore = xSemaphoreCreateMutexStatic((StaticSemaphore_t*) buf.data());
    }
    
    return xSemaphoreTake(_semaphore, timeout) == pdTRUE;
}

bool Mutex::unlock()
{
    return xSemaphoreGive(_semaphore) == pdTRUE;
}