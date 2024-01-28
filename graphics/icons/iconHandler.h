#pragma once

#include "icon.h"
#include "../../util/rate.h"
#include "../../util/hash.h"

class IconHandler
{
    public:
        Dimension2D<unsigned> dimensions;
        Point2D<unsigned> point;
        
        float maxBlinkBias = 0.9f;
        
        Rate blinkRate = Rate(1);
        Rate cycleRate = Rate(1);
        bool linearCycle = true;

        IconHandler();

        void addIcon(Icon& icon);
        void showIcon(Icon& icon);

        Icon* currentIcon();
                
        void addCycleIcon(Icon& icon);
        void startCycling();
        void stopCycling();
        bool isCycling();

        void startBlinking();
        void stopBlinking();
        bool isBlinking();

        void allocate();
        void deallocate();

        void draw(TFT_eSprite& sprite);
    
    protected:
        Hash<Icon*> _icons;
        std::vector<Icon*> _cycleIcons;
    
    private:
        unsigned _currentIcon = 0;
        bool _cycling = false;
        bool _blinking = false;
};
