#include "timer.h"
#include "../hal/hal.h"

Timer::Timer(unsigned ms)
{
    set(ms);
}

void Timer::set(unsigned ms)
{
    this->ms = ms;
    reset();
}

void Timer::reset()
{
    start = sysMillis();
    end = start + ms;
    silenced = false;
}

void Timer::ring()
{
    end = sysMillis();
    silenced = false;
}

void Timer::silence()
{
    silenced = true;
}

bool Timer::isRinging()
{
    return !silenced && sysMillis() >= end;
}

float Timer::getStage()
{
    if (!ms)
        return 1;
    float stage = (sysMillis() - start) / (float) ms;
    if (stage > 1)
        stage = 1;
    return stage;
}

float Timer::getStageSin()
{
    return (fsin(getStage() * 2 * fPI) + 1) / 2.0f;
}

float Timer::getStageCos()
{
    return (fcos(getStage() * 2 * fPI) + 1) / 2.0f;
}