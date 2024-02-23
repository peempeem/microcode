#include "button.h"
#include "../hal/hal.h"

Button::Button() : Mutex()
{

}

Button::Button(unsigned pin, bool activeLow, bool recordTaps, unsigned debounce, unsigned multitap) : Mutex(), pin(pin), recordTaps(recordTaps), debounce(debounce), multitap(multitap)
{
    pressed = activeLow ? 0 : 1;
    released = activeLow ? 1 : 0;
}

void Button::init()
{
    pinMode(pin, INPUT_PULLUP);
    lastState = digitalRead(pin);
}

void Button::update()
{
    if (debounce.isRinging())
    {
        unsigned val = digitalRead(pin);
        if (val == lastState)
        {
            if (recordTaps && currentTapEvent.taps && multitap.isRinging())
            {
                currentTapEvent.end = sysTime();
                lock();
                tapEvents.push(currentTapEvent);
                unlock();
                currentTapEvent = {0, 0, 0};
            }
        }
        else
        {
            if (val == pressed)
            {
                if (recordTaps)
                {
                    if (!currentTapEvent.taps)
                        currentTapEvent.start = sysTime();
                    currentTapEvent.taps++;
                    multitap.reset();
                }
            }
            debounce.reset();
            lastState = val;
            lastStageChange = sysTime();
        }
    }
}

bool Button::isPressed()
{
    return lastState == pressed;
}

bool Button::isReleased()
{
    return lastState == released;
}

bool Button::hasTapEvent()
{
    lock();
    bool hasEvent = !tapEvents.empty();
    unlock();
    return hasEvent;
}

Button::TapEvent Button::getTapEvent()
{
    lock();
    if (!tapEvents.empty())
    {
        TapEvent event = tapEvents.front();
        tapEvents.pop();
        unlock();
        return event;
    }
    unlock();
    return TapEvent();
}

void Button::clearTapEvents()
{
    lock();
    while (!tapEvents.empty())
        tapEvents.pop();
    unlock();
}

unsigned Button::pressedElapsedTime()
{
    if (isReleased())
        return 0;
    return millis() - lastStageChange;
}

unsigned Button::releasedElapsedTime()
{
    if (isPressed())
        return 0;
    return millis() - lastStageChange;
}
