#pragma once

#include "transitions.h"
#include "notifications.h"

class GUI
{
    public:
        Rate refereshRate = Rate(60);
        NotificationBar notifications;

        GUI();
        
        void init(unsigned rotation=2, unsigned notificationHeight=32, unsigned colorDepth=16);

        void display(bool on);

        bool containsScreen(Screen* screen);
        void addScreen(Screen* screen);

        void setMainScreen(Screen* screen);
        bool isMainScreen(Screen* screen);

        void transitionTo(Screen* screen, unsigned transition, unsigned ms);
        void update(bool sleep=false);
    
    private:
        TFT_eSPI* _display;
        TFT_eSprite* _sprite;
        TFT_eSprite* _notificationSprite;
        Dimension2D<unsigned> _dimensions;
        Hash<Screen*> _screens;
        Screen* _mainScreen = NULL;
        Screen* _transitionScreen = NULL;
        ScreenTransitioner _transitioner;
};
