#pragma once

#include "screen.h"
#include "../../util/timer.h"

#if __has_include(<TFT_eSPI.h>)

enum Transitions
{
    none,
    slideRight,
    slideLeft,
    slideUp,
    slideDown
};

class ScreenTransitioner
{
    public:
        ScreenTransitioner();

        void beginTransition(Screen* screen1, Screen* screen2, unsigned transition, unsigned ms);
        bool isTransitioning();
        void draw(TFT_eSprite& sprite);
    
    private:
        Screen* _screen1;
        Screen* _screen2;
        unsigned _transition;
        Timer _timer;
        bool _transitioning = false;
};

#endif
