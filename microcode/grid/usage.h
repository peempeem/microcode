#pragma once

#include "../util/rate.h"
#include <vector>

class UsageMeter
{
    public:
        UsageMeter(unsigned bins=5);

        void setLinkSpeed(unsigned linkSpeed);
        void update();
        void trackBytes(unsigned bytes);
        unsigned usage();
        uint64_t total();

    private:
        Rate _binCycle;
        uint64_t _last = 0;
        unsigned _linkSpeed = -1;
        unsigned _bin = 0;
        unsigned _bytesGone = 0;
        std::vector<unsigned> _bins;
        uint64_t _total = 0;

};
