#include "indicators.h"
#include "microcode/hal/hal.h"
#include <math.h>

ThrottleIndicator::ThrottleIndicator() : Indicator()
{
    std::vector<ColorGradient::ColorPosition> colors = 
    {
        ColorGradient::ColorPosition{RED, 0},
        ColorGradient::ColorPosition{YELLOW, 0.5f},
        ColorGradient::ColorPosition{GREEN, 1}
    };
    colorGradient.set(colors);
}

void ThrottleIndicator::draw(TFT_eSprite& sprite)
{
    sprite.drawRoundRect(
        center.x - dimensions.width / 2,
        center.y - dimensions.height / 2,
        dimensions.width,
        dimensions.height,
        3,
        borderColor.as16Bit()
    );

    float scale = dimensions.height / (float) lines;
    Point2D<float> p(
        round(center.x - lineWidth * dimensions.width / 2.0f),
        center.y - (dimensions.height - scale) / 2.0f
    );

    if (!enable)
    {
        float stage = pulse.getStage();
        for (unsigned i = 0; i < lines; i++)
        {
            float bias = (sin((stage + i / (float) (lines - 1)) * 2 * PI) + 1) / 2.0f;
            sprite.drawFastHLine(
                (unsigned) p.x,
                (unsigned) p.y,
                (unsigned) (lineWidth * dimensions.width),
                lineColorDark.blend(lineColorLight, bias).as16Bit()
            );
            p.y += scale;
        }
    }
    else
    {
        uint16_t c16 = colorGradient.at(power).as16Bit();
        for (unsigned i = 0; i < lines; i++)
        {
            float pos = 1 - i / (float) (lines - 1);
            if (power >= 0.5f && pos >= 0.5f && pos <= power || power <= 0.5f && pos <= 0.5f && pos >= power) 
            {
                sprite.drawFastHLine(
                    (unsigned) p.x, 
                    (unsigned) p.y, 
                    (unsigned) (lineWidth * dimensions.width), 
                    c16
                );
            }
            p.y += scale;
        }
    }
}

BoardBatteryIndicator::BoardBatteryIndicator() : Indicator()
{
    std::vector<ColorGradient::ColorPosition> colors = 
    {
        ColorGradient::ColorPosition{RED, 0.2f},
        ColorGradient::ColorPosition{YELLOW, 0.5f},
        ColorGradient::ColorPosition{GREEN, 0.8f}
    };
    colorGradient.set(colors);
}

void BoardBatteryIndicator::setBattery(float battery) {
    if (battery < 0)
        battery = 0;
    else if (battery > 1)
        battery = 1;
    _battery = battery;
}

void BoardBatteryIndicator::draw(TFT_eSprite& sprite)
{
    float scale = dimensions.width;
    float temp = scale * 2 * wheelVOffset + wheelHeight;
    if ((unsigned) temp > dimensions.height)
        scale *= dimensions.height / temp;

    // board
    float w = boardWidth * scale;
    float h = boardHeight * scale;
    float radius = boardRadius * scale;
    sprite.drawRoundRect(
        (unsigned) (center.x - w / 2.0f),
        (unsigned) (center.y - h / 2.0f),
        (unsigned) w,
        (unsigned) h,
        (unsigned) radius,
        lineColor.as16Bit()
    );

    w = deckWidth * scale;
    h = deckHeight * scale;
    sprite.fillRoundRect(
        (unsigned) (center.x - w / 2.0f),
        (unsigned) (center.y - h / 2.0f),
        (unsigned) w,
        (unsigned) h,
        (unsigned) radius,
        deckColor.as16Bit()
    );

    w = barWidth * _battery * scale;
    h = barHeight * scale;
    float ww = (barWidth * scale) / 2.0f;
    float hh = h / 2.0f;
    radius = boardRadius * scale;
    sprite.fillRoundRect(
        (unsigned) (center.x - ww),
        (unsigned) (center.y - hh),
        (unsigned) w,
        (unsigned) h,
        (unsigned) radius,
        colorGradient.at(_battery).as16Bit()
    );

    w = deckWidth * scale;
    h = deckHeight * scale;
    sprite.drawRoundRect(
        (unsigned) (center.x - w / 2.0f),
        (unsigned) (center.y - h / 2.0f),
        (unsigned) w,
        (unsigned) h,
        (unsigned) radius,
        lineColor.as16Bit()
    );

    // wheels
    float hOffset = wheelHOffset * scale;
    float vOffset = wheelVOffset * scale;
    w = wheelWidth * scale;
    h = wheelHeight * scale;
    radius = wheelRadius * scale;
    sprite.drawRoundRect(
        (unsigned) (center.x - hOffset - w / 2.0f),
        (unsigned) (center.y - vOffset - h / 2.0f),
        (unsigned) w,
        (unsigned) h,
        (unsigned) radius,
        lineColor.as16Bit()
    );
    sprite.drawRoundRect(
        (unsigned) (center.x + hOffset - w / 2.0f),
        (unsigned) (center.y - vOffset - h / 2.0f),
        (unsigned) w,
        (unsigned) h,
        (unsigned) radius,
        lineColor.as16Bit()
    );
    sprite.drawRoundRect(
        (unsigned) (center.x - hOffset - w / 2.0f),
        (unsigned) (center.y + vOffset - h / 2.0f),
        (unsigned) w,
        (unsigned) h,
        (unsigned) radius,
        lineColor.as16Bit()
    );
    sprite.drawRoundRect(
        (unsigned) (center.x + hOffset - w / 2.0f),
        (unsigned) (center.y + vOffset - h / 2.0f),
        (unsigned) w,
        (unsigned) h,
        (unsigned) radius,
        lineColor.as16Bit()
    );
}
