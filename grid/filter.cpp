#include "filter.h"
#include "../hal/hal.h"

//
//// IDFilter Class
//

IDFilter::IDFilter(unsigned timeout) : _timeout(timeout)
{

}

bool IDFilter::contains(unsigned id, bool add)
{
    if (_timestamps.contains(id))
        return true;
    
    if (add)
    {
        _timestamps[id] = sysTime() + _timeout;
        _next.push(id);
    }

    return false;
}

void IDFilter::preen()
{
    while (!_next.empty())
    {
        uint64_t* ts = _timestamps.at(_next.front());
        if (!ts || sysTime() > *ts)
        {
            _timestamps.remove(_next.front());
            _next.pop();
        }
        else
            break;
    }
}

unsigned IDFilter::size()
{
    return _timestamps.size();
}