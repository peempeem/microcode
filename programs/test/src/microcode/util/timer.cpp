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
    start = millis();
    end = start + ms;
    silenced = false;
}

void Timer::ring()
{
    end = millis();
    silenced = false;
}

void Timer::silence()
{
    silenced = true;
}

bool Timer::isRinging()
{
    return !silenced && millis() >= end;
}

float Timer::getStage()
{
    if (!ms)
        return 1;
    float stage = (millis() - start) / (float) ms;
    if (stage > 1)
        stage = 1;
    return stage;
}

float Timer::getStageSin()
{
    return (sin(getStage() * 2 * PI) + 1) / 2.0f;
}

float Timer::getStageCos()
{
    return (cos(getStage() * 2 * PI) + 1) / 2.0f;
}
