#include "rate.h"
#include "../hal/hal.h"

Rate::Rate(float hertz)
{
    setHertz(hertz);
}

void Rate::setMs(unsigned ms, bool keepStage)
{
    uint64_t newInverseRate;
    if (!ms)
        newInverseRate = -1;
    else
        newInverseRate = ms * 1000;
    
    start = sysTime();
    if (keepStage)
    {
        uint64_t offset = newInverseRate * getStage();
        if (offset > start)
            start = 0;
        else
            start -= offset;
    }

    inverseRate = newInverseRate;
    last = start;
}

void Rate::setHertz(float hertz, bool keepStage)
{
    uint64_t newInverseRate;
    if (hertz <= 0)
        newInverseRate = -1;
    else
        newInverseRate = 1e6 / hertz;
    
    start = sysTime();
    if (keepStage)
    {
        uint64_t offset = newInverseRate * getStage();
        if (offset > start)
            start = 0;
        else
            start -= offset;
    }

    inverseRate = newInverseRate;
    last = start;
}

void Rate::reset()
{
    start = sysTime();
    last = start;
}

bool Rate::isReady()
{
    uint64_t time = sysTime();
    if (time > inverseRate + last)
    {
        last = time - (time - last) % inverseRate;
        return true;
    }
    return false;
}

void Rate::sleep()
{
    sysSleep((inverseRate - (sysTime() - start) % inverseRate) / (unsigned) 1e3);
}

float Rate::getStage()
{
    return ((sysTime() - last) % inverseRate) / (float) inverseRate;
}

float Rate::getStageSin(float offset)
{
    return (sin((getStage() + offset) * 2 * PI) + 1) / 2.0f;
}

float Rate::getStageCos(float offset)
{
    return (cos((getStage() + offset) * 2 * PI) + 1) / 2.0f;
}
