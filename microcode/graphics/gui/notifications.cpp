#include "notifications.h"

#if __has_include(<TFT_eSPI.h>)

NotificationBar::NotificationBar()
{

}

void NotificationBar::addIcon(IconHandler* icon)
{
    _icons.push_back(icon);
}

void NotificationBar::removeIcon(IconHandler* icon)
{
    for (auto it = _icons.begin(); it != _icons.end(); it++)
    {
        if (*it == icon)
        {
            _icons.erase(it);
            break;
        }
    }
}

void NotificationBar::draw(TFT_eSprite& sprite)
{
    sprite.fillRect(0, 0, dimensions.width, dimensions.height, backgroundColor.as16Bit());
    int x = (int) dimensions.width - (int) dimensions.height;
    for (IconHandler* icon : _icons)
    {
        icon->dimensions.set(dimensions.height, dimensions.height);
        icon->point.set(x, 0);
        icon->draw(sprite);
        x -= dimensions.height;
    }
}

#endif

