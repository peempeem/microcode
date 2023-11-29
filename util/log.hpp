#include "log.h"
#include "../hal/hal.h"
#include <string.h>


template <class T> static void log(const char* header, T data, bool nl) {
    unsigned time = sysMillis();
    sysPrint('[');
    sysPrint(time / 1000);
    sysPrint('.');
    unsigned ms = time % 1000;
    if (ms < 100) 
    {
        sysPrint('0');
        if (ms < 10)
            sysPrint('0');
    }
    sysPrint(ms);
    sysPrint("] [");
    sysPrint(xPortGetCoreID());
    sysPrint('|');
    unsigned priority = uxTaskPriorityGet(NULL);
    if (priority < 10)
        sysPrint(' ');
    sysPrint(priority);
    sysPrint("] [");
    sysPrint(header);
    sysPrint("]: ");
    sysPrint(data);
    if (nl)
        sysPrint('\n');
}

template <class T> static void logc(T data, bool nl)
{
    sysPrint(data, nl);
}

template <class T> static void logx(T data, bool nl)
{
    sysPrintHex(data, nl);
}

static void logf()
{
    sysPrint(" -> [ FAILED ]", true);
}

static void logs()
{
    sysPrint(" -> [ SUCCESS ]", true);
}
