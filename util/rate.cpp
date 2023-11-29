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
    
    start = sysMicros();
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
    
    start = sysMicros();
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
    start = sysMicros();
    last = start;
}

bool Rate::isReady()
{
    uint64_t time = sysMicros();
    if (time > inverseRate + last)
    {
        last = time - (time - last) % inverseRate;
        return true;
    }
    return false;
}

void Rate::sleep()
{
    sysSleep((inverseRate - (micros() - start) % inverseRate) / (unsigned) 1e3);
}

float Rate::getStage()
{
    return ((sysMicros() - last) % inverseRate) / (float) inverseRate;
}

float Rate::getStageSin(float offset)
{
    return (fsin((getStage() + offset) * 2 * fPI) + 1) / 2.0f;
}

float Rate::getStageCos(float offset)
{
    return (fcos((getStage() + offset) * 2 * fPI) + 1) / 2.0f;
}
