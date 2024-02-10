#include "screens.h"
#include "microcode/hal/hal.h"

BlackScreen::BlackScreen()
{

}

bool BlackScreen::draw(TFT_eSprite& sprite)
{
    if (!_visable)
        return false;
    Color c = BLACK;
    sprite.fillRect(point.x, point.y, dimensions.width, dimensions.height, c.as16Bit());
    return true;
}

BootScreen::BootScreen() : Screen()
{

}

void BootScreen::setVisability(bool visable)
{
    Screen::setVisability(visable);
    _fadeInTimer.set(1000);
    _circleTimer.set(250);
}

bool BootScreen::draw(TFT_eSprite& sprite)
{
    if (!Screen::draw(sprite))
        return false;
    
    Point2D<unsigned> c = Point2D<unsigned>(dimensions.width / 2, dimensions.height / 2);

    float dr = -360 / (float) spokes;
    float spinAngle = (unsigned) (spokes * spinRate.getStage()) * dr;
    float stage = _fadeInTimer.getStage();

    for (unsigned spoke = 0; spoke < spokes; spoke++)
    {
        float angle = dr * spoke + spinAngle + startAngle;
        float xRot = cos(angle * PI / 180.0f);
        float yRot = sin(angle * PI / 180.0f);
        unsigned sx = c.x + circleMin * xRot;
        unsigned sy = c.y + circleMin * yRot;
        unsigned ex = c.x + circleMax * xRot;
        unsigned ey = c.y + circleMax * yRot;
        sprite.drawLine(sx, sy, ex, ey, accentColor.blend(mainColor, (spoke + 1) / (float) spokes).blend(BLACK, 1 - stage).as16Bit());
    }
    return true;
}

ConnectScreen::ConnectScreen() : Screen()
{
    wifi.cycleRate.setHertz(0.25f);
}

void ConnectScreen::setVisability(bool visable)
{
    Screen::setVisability(visable);

    if (!visable)
    {
        wifi.deallocate();
        board.deallocate();
    }
}

bool ConnectScreen::draw(TFT_eSprite& sprite)
{    
    if (!Screen::draw(sprite))
        return false;

    Point2D<unsigned> center = Point2D<unsigned>(dimensions.width / 2, dimensions.height / 2);

    unsigned scale = scaleDimension();
    unsigned padding = scale * 0.15f;
    wifi.dimensions.width = scale * 0.7f;
    wifi.dimensions.height = wifi.dimensions.width;
    wifi.point.set(center.x - wifi.dimensions.width / 2, padding);
    wifi.draw(sprite);

    board.dimensions.width = scale * 0.95f;
    board.dimensions.height = board.dimensions.width * (board.currentIcon()->height() / (float) board.currentIcon()->width());
    board.point.set(center.x - board.dimensions.width / 2, wifi.point.y + wifi.dimensions.height + padding);
    board.draw(sprite);

    sprite.setFreeFont(&Orbitron_Light_24);
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(textColor.as16Bit());
    sprite.drawString(text.c_str(), center.x, board.point.y + board.dimensions.height + padding, 1);
    return true;
}

void HomeScreen::setSpeed(float speed) {
    _speed = String(fabs(speed), 1);
}

void HomeScreen::setSpeedMode(int mode)
{
    _speedMode = "M: " + String(mode);
}

void HomeScreen::setMemoryUsage(unsigned memoryUsagePercent)
{
    _mem = String(memoryUsagePercent) + " %"; 
}

void HomeScreen::setVisability(bool visable)
{
    Screen::setVisability(visable);
}

bool HomeScreen::draw(TFT_eSprite& sprite)
{
    if (!Screen::draw(sprite))
        return false;
    
    throttle.dimensions.set(dimensions.width * 0.2f, dimensions.height * 0.9f);
    throttle.center.set(throttle.dimensions.width / 2 + dimensions.width * 0.05f, throttle.dimensions.height / 2 + dimensions.height * 0.05f);
    throttle.draw(sprite);

    boardBattery.dimensions.set(dimensions.width * 0.7f, dimensions.height * 0.2f);
    boardBattery.center.set(boardBattery.dimensions.width / 2 + dimensions.width * 0.3f, boardBattery.dimensions.height / 2 + dimensions.height * 0.8f);
    boardBattery.draw(sprite);

    sprite.setFreeFont(&Orbitron_Light_32);
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(mainTextColor.as16Bit());
    Point2D<unsigned> center(boardBattery.center.x, dimensions.height * 0.15f);
    sprite.drawString(
        _speed.c_str(),
        center.x,
        center.y,
        1
    );

    sprite.setFreeFont(&Orbitron_Light_24);
    sprite.setTextColor(accentTextColor.as16Bit());
    center.y = dimensions.height * 0.3f;
    sprite.drawString(
        _speedDesc.c_str(),
        center.x,
        center.y,
        1
    );

    sprite.setTextColor(mainTextColor.as16Bit());
    center.y = dimensions.height * 0.45f;
    sprite.drawString(
        _speedMode.c_str(),
        center.x,
        center.y,
        1
    );

    sprite.setTextColor(accentTextColor.as16Bit());
    center.y = dimensions.height * 0.6f;
    sprite.drawString(
        _mem.c_str(),
        center.x,
        center.y,
        1
    );
    return true;
}