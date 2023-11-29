#include "color.h"
#include <algorithm>

bool c15IsTransparent(uint16_t c15)
{
    return (int16_t) c15 >= 0;
}

uint16_t cvtC15ToC16(uint16_t c15)
{
    uint16_t c16 = (c15 & 0x7c00) << 1;
    c16 += ((((c15 & 0x03e0) >> 5) * 63) / 31) << 5;
    c16 += c15 & 0x1f;
    return c16;
}

Color::Color() : r(0), g(0), b(0)
{

}

Color::Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b)
{

}

Color::Color(uint16_t c16)
{
    b = ((c16 & 0x1f) * 255) / 31;
    g = (((c16 >> 5) & 0x3f) * 255) / 63;
    r = (((c16 >> 11) & 0x1f) * 255) / 31;
}

bool Color::compareRGB(const Color& other)
{
    return r == other.r && g == other.g && b == other.b;
}

bool Color::compare16b(const Color& other)
{
    return as16Bit() == other.as16Bit();
}

Color Color::color()
{
    return Color(r, g, b);
}

uint16_t Color::as16Bit() const
{
    return (((((((unsigned) r) * 31) / 255) << 6) + (((unsigned) g) * 63) / 255) << 5) + (((unsigned) b) * 31) / 255;
}

Color Color::blend(const Color& other, float bias)
{
    if (bias <= 0)
        return color();
    else if (bias >= 1)
        return other;
    return Color(
        r + (int) (((int) other.r - (int) r) * bias),
        g + (int) (((int) other.g - (int) g) * bias),
        b + (int) (((int) other.b - (int) b) * bias)
    );
}

Color Color::blend(uint16_t c16, float bias)
{
    Color other = Color(c16);
    return blend(other, bias);
}

bool ColorGradient::ColorPosition::operator<(const ColorPosition& other) const
{
    return position < other.position;
}

bool ColorGradient::ColorPosition::operator>(const ColorPosition& other) const
{
    return position > other.position;
}

bool ColorGradient::ColorPosition::operator==(const ColorPosition& other) const
{
    return position == other.position;
}

ColorGradient::ColorGradient()
{
    
}

void ColorGradient::set(std::vector<ColorPosition> colors)
{
    if (!colors.empty())
        std::sort(colors.begin(), colors.end());
    _colors = colors;
}

Color ColorGradient::at(float position)
{
    if (_colors.empty())
        return Color();
    
    int idx = -1;
    for (unsigned i = 0; i < _colors.size(); i++)
    {
        if (_colors[i].position > position)
        {
            idx = i;
            break;
        }
        else if (_colors[i].position == position)
            return _colors[i].color;
    }
    if (idx == -1)
        return _colors.back().color;
    else if (idx == 0)
        return _colors.front().color;

    float dist = _colors[idx].position - _colors[idx - 1].position;
    if (dist == 0)
        return _colors[idx].color;
    
    return _colors[idx - 1].color.blend(
        _colors[idx].color, 
        (position - _colors[idx - 1].position) / dist
    );
}