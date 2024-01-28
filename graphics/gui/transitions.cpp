#include "transitions.h"
#include "../../hal/hal.h"

#if __has_include(<TFT_eSPI.h>)

ScreenTransitioner::ScreenTransitioner()
{

}

void ScreenTransitioner::beginTransition(Screen* screen1, Screen* screen2, unsigned transition, unsigned ms)
{
    if (isTransitioning())
        return;
    
    _screen1 = screen1;
    _screen2 = screen2;
    _transition = transition;
    _timer.set(ms);
    _transitioning = true;
}

bool ScreenTransitioner::isTransitioning()
{
    return _transitioning;
}

void ScreenTransitioner::draw(TFT_eSprite& sprite)
{
    if (_timer.isRinging())
        _transitioning = false;
    
    float stage = sin(_timer.getStage() * PI / 2.0f);

    if (_screen2 != NULL)
    {
        _screen2->draw(sprite);
        switch (_transition)
        {
            case Transitions::none:
            {
                sprite.pushSprite(_screen2->point.x, _screen2->point.y);
                _timer.ring();
            }

            case Transitions::slideRight:
            {
                sprite.pushSprite((int) (_screen2->dimensions.width * (1 - stage)) + _screen2->point.x, _screen2->point.y);
                break;
            }

            case Transitions::slideLeft:
            {
                sprite.pushSprite((int) (_screen2->dimensions.width * (stage - 1)) + _screen2->point.x, _screen2->point.y);
                break;
            }

            case Transitions::slideUp:
            {
                sprite.pushSprite(_screen2->point.x, (int) (_screen2->dimensions.height * (1 - stage)) + _screen2->point.y);
                break;
            }

            case Transitions::slideDown:
            {
                sprite.pushSprite(_screen2->point.x, (int) (_screen2->dimensions.height * (stage - 1)) + _screen2->point.y);
                break;
            }
        }
    }
}

#endif
