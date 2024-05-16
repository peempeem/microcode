#pragma once

#include "microcode/graphics/icons/iconHandler.h"

class BatteryIcon : public IconHandler
{
    public:
        Icon empty = Icon("/icons/BatteryEmpty.icon");
        Icon percent25 = Icon("/icons/Battery25.icon");
        Icon percent50 = Icon("/icons/Battery50.icon");
        Icon percent75 = Icon("/icons/Battery75.icon");
        Icon full = Icon("/icons/BatteryFull.icon");;

        BatteryIcon() : IconHandler()
        {
            addIcon(empty);
            addIcon(percent25);
            addIcon(percent50);
            addIcon(percent75);
            addIcon(full);
            showIcon(full);
        }
};

class WiFiIcon : public IconHandler
{
    public:
        Icon wifiNone = Icon("/icons/WiFiNone.icon");
        Icon wifi1 = Icon("/icons/WiFi1.icon");
        Icon wifi2 = Icon("/icons/WiFi2.icon");
        Icon wifi3 = Icon("/icons/WiFi3.icon");
        Icon wifiFull = Icon("/icons/WiFiFull.icon");
        Icon pairing = Icon("/icons/Pairing.icon");

        WiFiIcon() : IconHandler()
        {
            addIcon(wifiNone);
            addIcon(wifi1);
            addIcon(wifi2);
            addIcon(wifi3);
            addIcon(wifiFull);
            addIcon(pairing);

            addCycleIcon(wifi1);
            addCycleIcon(wifi2);
            addCycleIcon(wifi3);
            addCycleIcon(wifiFull);

            showIcon(wifiNone);
            linearCycle = false;
        };
};

class SkateboardIcon : public IconHandler
{
    public:
        Icon skateboard = Icon("/icons/Skateboard.icon");

        SkateboardIcon() : IconHandler()
        {
            addIcon(skateboard);
            showIcon(skateboard);
        };
};