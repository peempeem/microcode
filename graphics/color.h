#pragma once

#include <stdint.h>
#include <vector>

bool c15IsTransparent(uint16_t c15);
uint16_t cvtC15ToC16(uint16_t c15);

class Color
{
    public:
        uint8_t r;
        uint8_t g;
        uint8_t b;

        Color();
        Color(uint8_t r, uint8_t g, uint8_t b);
        Color(uint16_t c16);

        bool compareRGB(const Color& other);
        bool compare16b(const Color& other);

        Color color();
        uint16_t as16Bit() const;
        Color blend(const Color& other, float bias);
        Color blend(uint16_t c16, float bias);
};

const static Color BLACK        (0,     0,      0   );
const static Color WHITE        (255,   255,    255 );
const static Color GRAY         (100,   100,    100 );
const static Color GREEN        (0,     255,    0   );
const static Color YELLOW       (255,   255,    0   );
const static Color RED          (255,   0,      0   );
const static Color FOREST_GREEN (0,     220,    60  );
const static Color DARK_GRAY    (20,    20,     20  );
const static Color LIGHT_GRAY   (200,   200,    200 );
const static Color LIGHTER_GRAY (145,   145,    145 );
const static Color ALMOST_BLACK (9,     5,      9   );
const static Color MAROON       (130,   0,      0   );
const static Color TURQUOISE    (64,    224,    208 );
const static Color SUNSET_ORANGE(238,   175,    97  );
const static Color SUNSET_PURPLE(106,   13,     131 );
const static Color CERULEAN     (0,     171,    240 );

class ColorGradient
{
    public:
        struct ColorPosition
        {
            Color color;
            float position;

            bool operator<(const ColorPosition& other) const;
            bool operator>(const ColorPosition& other) const;
            bool operator==(const ColorPosition& other) const;
        };

        ColorGradient();
        
        void set(std::vector<ColorPosition> colors);
        Color at(float position);

    private:
        std::vector<ColorPosition> _colors;
};
