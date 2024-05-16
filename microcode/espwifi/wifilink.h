#pragma once

#include "maclink.h"
#include <list>

class WiFiLink
{
    public:
        WiFiLink();

        bool init(const char* name, unsigned random=10);

        void setClockSpeed(unsigned megahertz);

        bool add(MACLink& maclink);
        bool contains(MACLink& maclink);
        void remove(MACLink& maclink);

        void update();

        bool scan(const char* key);
        bool isScanning();
        std::list<MAC> getScans();
        void emptyScans();

        void setPairing(bool pairing);
        bool isPairing();
        std::list<MAC> getPairings();
    
    private:
        bool _initialized = false;
        bool _scanning = false;
        bool _pairing = false;
        std::list<MAC> scans;
        std::list<MAC> pairings;
        std::list<MACLink*> maclinks;
        std::string _key;

        bool _get(MAC& mac, std::list<MACLink*>::iterator& it);
        bool _get(MACLink& maclink, std::list<MACLink*>::iterator& it);
} static wifilink;
