#pragma once

#include "../drawable.h"
#include "../icons/iconHandler.h"
#include <list>

#if __has_include(<TFT_eSPI.h>)

class NotificationBar
{
    public:
        Dimension2D<unsigned> dimensions;
        Color backgroundColor = ALMOST_BLACK;

        NotificationBar();

        void addIcon(IconHandler* icon);
        void removeIcon(IconHandler* icon);
        void draw(TFT_eSprite& sprite);

    private:
        std::list<IconHandler*> _icons;      
};

#endif

