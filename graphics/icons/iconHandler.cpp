#include "iconHandler.h"

#if __has_include(<TFT_eSPI.h>)

IconHandler::IconHandler()
{

}

void IconHandler::addIcon(Icon& icon)
{
    _icons[(unsigned) &icon] = &icon;
}

void IconHandler::showIcon(Icon& icon)
{
    if (!_icons.contains((unsigned) &icon))
        return;
    _currentIcon = (unsigned) &icon;
    icon.allocate();
}

Icon* IconHandler::currentIcon()
{
    return _icons[_currentIcon];
}

void IconHandler::addCycleIcon(Icon& icon)
{
    if (!_icons.contains((unsigned) &icon))
        return;
    _cycleIcons.push_back(&icon);
}

void IconHandler::startCycling()
{
    if (!_cycling && _cycleIcons.size() > 0)
    {
        _cycling = true;
        cycleRate.reset();
    }
}

void IconHandler::stopCycling()
{
    _cycling = false;
}

bool IconHandler::isCycling()
{
    return _cycling;
}

void IconHandler::startBlinking()
{
    _blinking = true;
    blinkRate.reset();
}

void IconHandler::stopBlinking()
{
    _blinking = false;
}

bool IconHandler::isBlinking()
{
    return _blinking;
}

void IconHandler::allocate()
{
    for (Icon* icon : _icons)
        icon->allocate();
}

void IconHandler::deallocate()
{
    for (Icon* icon : _icons)
        icon->deallocate();
}

void IconHandler::draw(TFT_eSprite& sprite)
{
    if (!_cycling && !_currentIcon)
        return;
    
    float blinkBias;
    float cycleBias;
    Icon* icon;
    Icon* cIcon;

    if (_blinking)
        blinkBias = maxBlinkBias * blinkRate.getStageSin();
    
    if (_cycling)
    {
        float stage = cycleRate.getStage();
        float sp = stage * _cycleIcons.size();
        unsigned s = (unsigned) (sp);
        cycleBias = sp - s;
        
        if (!linearCycle)
            cycleBias = 1 / (1 + expf(-10 * (cycleBias - 0.5f)));
        
        if (s == _cycleIcons.size())
            s = 0;
        
        icon = _cycleIcons[s];
        cIcon = (s < _cycleIcons.size() - 1) ? _cycleIcons[s + 1] : _cycleIcons[0];
        cIcon->allocate();
    } else
        icon = _icons[_currentIcon];
    icon->allocate();

    if (!icon->allocated())
        return;

    if (!_cycling || !cIcon->allocated())
    {   
        for (unsigned dy = 0; dy < dimensions.height; dy++)
        {
            unsigned sy = (dy * icon->height()) / dimensions.height;
            unsigned ddy = point.y + dy;
            for (unsigned dx = 0; dx < dimensions.width; dx++)
            {
                unsigned sx = (dx * icon->width()) / dimensions.width;
                unsigned ddx = point.x + dx;
                uint16_t color = icon->at(sx, sy);
                if (!c15IsTransparent(color))
                {
                    color = cvtC15ToC16(color);
                    if (_blinking)
                        color = Color(color).blend(sprite.readPixel(ddx, ddy), blinkBias).as16Bit();
                    sprite.drawPixel(ddx, ddy, color);
                }
            }
        }
    }
    else
    {
        for (unsigned dy = 0; dy < dimensions.height; dy++)
        {
            unsigned sy1 = (dy * icon->height()) / dimensions.height;
            unsigned sy2 = (dy * cIcon->height()) / dimensions.height;
            int ddy = point.y + dy;
            for (unsigned dx = 0; dx < dimensions.width; dx++)
            {
                unsigned sx1 = (dx * icon->width()) / dimensions.width;
                unsigned sx2 = (dx * cIcon->width()) / dimensions.width;
                int ddx = point.x + dx;
                uint16_t color1 = icon->at(sx1, sy1);
                uint16_t color2 = cIcon->at(sx2, sy2);
                bool t1 = c15IsTransparent(color1);
                bool t2 = c15IsTransparent(color2);
                
                if (t1 && t2)
                    continue;
                
                Color color;
                Color backgroundColor = Color(sprite.readPixel(ddx, ddy));

                if (t1)
                    color = Color(cvtC15ToC16(color2)).blend(backgroundColor, cycleBias);
                else if (t2)
                    color = Color(cvtC15ToC16(color1)).blend(backgroundColor, cycleBias);
                else
                    color = Color(cvtC15ToC16(color1)).blend(cvtC15ToC16(color2), cycleBias);

                if (_blinking)
                    color = color.blend(backgroundColor, blinkBias);
                
                sprite.drawPixel(ddx, ddy, color.as16Bit());
            }
        }
    }
}

#endif
