#include "usage.h"
#include "../hal/hal.h"

UsageMeter::UsageMeter(unsigned bins)
{
    _bins = std::vector<unsigned>(bins);
}

void UsageMeter::setLinkSpeed(unsigned linkSpeed)
{
    _linkSpeed = linkSpeed;
}

void UsageMeter::update()
{
    if (!_last)
        _last = sysTime();
    
    if (_binCycle.isReady())
    {
        if (++_bin >= _bins.size())
            _bin = 0;

        _bytesGone += _linkSpeed * (sysTime() - _last) / (float) 1e6;
        _last = sysTime();

        unsigned u = usage();
        if (_bytesGone > u)
            _bytesGone = u;
        
        if (_bins[_bin] < _bytesGone)
        {
            _bytesGone -= _bins[_bin];
            _bins[_bin] = 0;
        }
        else
        {
            _bins[_bin] -= _bytesGone;
            _bytesGone = 0;
        }
    }
}

void UsageMeter::trackBytes(unsigned bytes)
{
    _bins[_bin] += bytes;
    _total += bytes;
}

unsigned UsageMeter::usage()
{
    unsigned u = 0;
    for (unsigned bytes : _bins)
        u += bytes;
    return u;
}

uint64_t UsageMeter::total()
{
    return _total;
}
