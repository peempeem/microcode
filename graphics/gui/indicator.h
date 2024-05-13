#pragma once

#include "../drawable.h"

#if __has_include(<TFT_eSPI.h>)

class Indicator
{
    public:
        Dimension2D<unsigned> dimensions;
        Point2D<unsigned> center;

        virtual void draw(TFT_eSprite& sprite);
    
    protected:
        virtual float fWidth();
        virtual float fHeight();
};

#endif
