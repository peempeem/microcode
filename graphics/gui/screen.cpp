#include "screen.h"

#if __has_include(<TFT_eSPI.h>)

Screen::Screen()
{
    _triangles = Triangles(16);
}

unsigned Screen::scaleDimension()
{
    return (dimensions.width < dimensions.height) ? dimensions.width : dimensions.height;
}

void Screen::setVisability(bool visable)
{
    _visable = visable;
}

bool Screen::draw(TFT_eSprite& sprite)
{
    if (!_visable)
        return false;

    sprite.fillRect(0, 0, dimensions.width, dimensions.height, backgroundColor.as16Bit());

    unsigned cs = (unsigned) (cornerSize * scaleDimension());
    uint16_t cc = cornerColor.as16Bit();
    sprite.drawFastVLine(0, 0, cs, cc);
    sprite.drawFastHLine(0, 0, cs, cc);
    sprite.drawFastVLine(dimensions.width - 1, 0, cs, cc);
    sprite.drawFastHLine(dimensions.width - cs, 0, cs, cc);
    sprite.drawFastVLine(0, dimensions.height - cs, cs, cc);
    sprite.drawFastHLine(0, dimensions.height - 1, cs, cc);
    sprite.drawFastVLine(dimensions.width - 1, dimensions.height - cs, cs, cc);
    sprite.drawFastHLine(dimensions.width - cs, dimensions.height - 1, cs, cc);

    float xs = dimensions.width / (float) dots;
    float ys = dimensions.height / (float) dots;
    float stage = dotsRate.getStageSin();
    for (unsigned y = 0; y < dots; y++)
    {
        float yy = ((y + 0.5f) * ys);
        float blend = 1 - fabs(stage - yy / (float) dimensions.height);
        Color intermColor = dotsDefaultColor.blend(dotsBlendColor, blend);
        for (unsigned x = 0; x < dots; x++)
        {
            float xx = ((x + 0.5f) * xs);
            blend = 1 - fabs(stage - xx / (float) dimensions.width);
            sprite.drawPixel((unsigned) xx, (unsigned) yy, intermColor.blend(dotsBlendColor, blend).as16Bit());
        }
    }

    _triangles.dimensions = dimensions;
    _triangles.point.set(0, 0);
    _triangles.draw(sprite);
    return true;
}

#endif
