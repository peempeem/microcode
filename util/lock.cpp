#include "lock.h"

Lock::Lock()
{
	#if __has_include(<esp.h>)
	spinlock = portMUX_INITIALIZER_UNLOCKED;
	#endif
}

void Lock::lock()
{
	#if __has_include(<esp.h>)
	portENTER_CRITICAL_SAFE(&spinlock);
	#else
	portENTER_CRITICAL();
	#endif
}

void Lock::unlock()
{
	#if __has_include(<esp.h>)
	portEXIT_CRITICAL_SAFE(&spinlock);
	#else
	portENTER_CRITICAL();
	#endif
}
