#pragma once

#include "triangles.h"

#if __has_include(<TFT_eSPI.h>)

class Screen
{
    public:
        Dimension2D<unsigned> dimensions;
        Point2D<int> point;
        Color backgroundColor = BLACK;
        Color cornerColor = GRAY;
        Color dotsDefaultColor = LIGHT_GRAY;
        Color dotsBlendColor = ALMOST_BLACK;
        float cornerSize = 0.1f;
        unsigned dots = 15;
        Rate dotsRate = Rate(0.4f);

        Screen();

        unsigned scaleDimension();
        virtual void setVisability(bool visable);
        virtual bool draw(TFT_eSprite& sprite);
    
    protected:
        Triangles _triangles;
        bool _visable = true;
};

#endif
