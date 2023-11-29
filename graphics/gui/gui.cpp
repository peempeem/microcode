#include "gui.h"

#if __has_include(<TFT_eSPI.h>)

GUI::GUI()
{
    
}

void GUI::init(unsigned rotation, unsigned notificationHeight, unsigned colorDepth)
{
    _display = new TFT_eSPI();
    _display->init();
    _display->setRotation(rotation);
    _display->fillScreen(0);

    (rotation % 2) ? _dimensions.set(TFT_HEIGHT, TFT_WIDTH) : _dimensions.set(TFT_WIDTH, TFT_HEIGHT);

    notifications.dimensions.set(_dimensions.width, notificationHeight);
    _dimensions.height -= notificationHeight;

    _sprite = new TFT_eSprite(_display);
    _sprite->setColorDepth(colorDepth);
    _sprite->createSprite(_dimensions.width, _dimensions.height);

    _notificationSprite = new TFT_eSprite(_display);
    _notificationSprite->setColorDepth(colorDepth);
    _notificationSprite->createSprite(notifications.dimensions.width, notifications.dimensions.height);
}

void GUI::display(bool on)
{
    _display->writecommand(on ? ST7789_DISPON : ST7789_DISPOFF);
    _display->writecommand(on ? ST7789_SLPOUT : ST7789_SLPIN);
}

bool GUI::containsScreen(Screen* screen)
{
    return _screens.at((unsigned) screen);
}

void GUI::addScreen(Screen* screen)
{
    _screens.insert((unsigned) screen, screen);
    screen->dimensions = _dimensions;
    screen->point.set(0, notifications.dimensions.height);
}

void GUI::setMainScreen(Screen* screen)
{
    if (_screens.contains((unsigned) screen))
    {
        screen->setVisability(true);
        _mainScreen = screen;
    }
}

bool GUI::isMainScreen(Screen* screen)
{
    return _mainScreen == screen;
}

void GUI::transitionTo(Screen* screen, unsigned transition, unsigned ms)
{
    if (_screens.contains((unsigned) screen) && screen != _mainScreen && !_transitioner.isTransitioning())
    {
        _transitioner.beginTransition(_mainScreen, screen, transition, ms);
        if (_mainScreen)
            _mainScreen->setVisability(false);
        setMainScreen(screen);
    }
}

void GUI::update(bool sleep)
{
    if (sleep || refereshRate.isReady())
    {
        if (_transitioner.isTransitioning())
            _transitioner.draw(*_sprite);
        else
        {
            if (_mainScreen)
                _mainScreen->draw(*_sprite);
            _sprite->pushSprite(_mainScreen->point.x, _mainScreen->point.y);
        }

        notifications.draw(*_notificationSprite);
        _notificationSprite->pushSprite(0, 0);
        
        if (sleep)
            refereshRate.sleep();
    }
}

#endif
