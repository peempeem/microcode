#pragma once

#include "microcode/graphics/gui/indicator.h"
#include "microcode/util/rate.h"

class ThrottleIndicator : public Indicator
{
    public:
        Color borderColor = LIGHT_GRAY;
        Color lineColorDark = ALMOST_BLACK;
        Color lineColorLight = LIGHTER_GRAY;
        ColorGradient colorGradient;
        unsigned lines = 28;
        float lineWidth = 0.7f;
        Rate pulse = Rate(0.5f);
        float power = 0.0f;
        bool enable = false;

        ThrottleIndicator();

        void draw(TFT_eSprite& sprite);    
};

class BoardBatteryIndicator : public Indicator
{
    public:
        Color lineColor = WHITE;
        Color deckColor = DARK_GRAY;
        ColorGradient colorGradient;
        float boardWidth = 1.0f;
        float boardHeight = 0.17f;
        float boardRadius = 0.05f;
        float deckWidth = 0.7f;
        float deckHeight = 0.24;
        float barWidth = 0.65f;
        float barHeight = 0.19f;
        float wheelHOffset = 0.4f;
        float wheelVOffset = 0.14f;
        float wheelWidth = 0.1f;
        float wheelHeight = 0.07f;
        float wheelRadius = 0.02f;

        BoardBatteryIndicator();

        void setBattery(float battery);
        void draw(TFT_eSprite& sprite);

    private:
        float _battery = 0.0f;
};
