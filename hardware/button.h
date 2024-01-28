#pragma once

#include "../util/timer.h"
#include "../util/lock.h"
#include <queue>

class Button : private BinarySemaphore
{
    public:
        struct TapEvent
        {
            unsigned taps;
            unsigned start;
            unsigned end;
        };
        
        Button();
        Button(unsigned pin, bool activeLow=true, bool recordTaps=true, unsigned debounce=5, unsigned multitap=300);

        void init();
        void update();

        bool isPressed();
        bool isReleased();
        
        bool hasTapEvent();
        TapEvent getTapEvent();
        void clearTapEvents();

        unsigned pressedElapsedTime();
        unsigned releasedElapsedTime();

    private:
        unsigned pin;
        bool recordTaps;
        unsigned pressed;
        unsigned released;
        unsigned lastState;
        unsigned lastStageChange;

        TapEvent currentTapEvent{0, 0, 0};
        std::queue<TapEvent> tapEvents;

        Timer debounce;
        Timer multitap;
};
