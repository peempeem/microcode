#include "spinlock.h"

//
//// SpinLock Class
//

bool SpinLock::lock(BaseType_t waitCycles)
{
    return xPortEnterCriticalTimeoutSafe(&_spinlock, waitCycles) == pdPASS;
}

void SpinLock::unlock()
{
    vPortExitCriticalSafe(&_spinlock);
}
