#pragma once

#include "microcode/graphics/gui/screen.h"
#include "microcode/util/timer.h"
#include "icons.h"
#include "indicators.h"

class BlackScreen : public Screen
{
    public:
        BlackScreen();

        bool draw(TFT_eSprite& sprite);
};

class BootScreen : public Screen
{
    public:
        Rate spinRate = Rate(1);
        float circleMin = 30.0f;
        float circleMax = 60.0f;

        Color mainColor = SUNSET_ORANGE;
        Color accentColor = SUNSET_PURPLE;

        float startAngle = -90.0f;

        unsigned spokes = 30;
        unsigned circleSpace = 13;

        BootScreen();

        void setVisability(bool visable);
        bool draw(TFT_eSprite& sprite);

    private:
        Timer _fadeInTimer;
        Timer _circleTimer;
};

class ConnectScreen : public Screen
{
    public:
        Color textColor = GREEN;
        String text;

        WiFiIcon wifi;
        SkateboardIcon board;

        ConnectScreen();

        void setVisability(bool visable);
        bool draw(TFT_eSprite& sprite);
};

class HomeScreen : public Screen
{
    public:
        ThrottleIndicator throttle;
        BoardBatteryIndicator boardBattery;
        
        Color mainTextColor = WHITE;
        Color accentTextColor = GRAY;

        void setSpeed(float speed);
        void setSpeedMode(int mode);
        void setMemoryUsage(unsigned memoryUsagePercent);

        void setVisability(bool visable);
        bool draw(TFT_eSprite& sprite);
    
    private:
        String _speed = "0.0";
        String _speedDesc = "mph";
        String _speedMode = "M: 0";
        String _mem = "0.0 %";
};
