#include "eventScheduler.h"

EventScheduler::EventScheduler()
{

}

bool EventScheduler::has(unsigned event)
{
    return _trackedEvents.contains(event);
}

void EventScheduler::schedule(unsigned event, unsigned ms, bool overwrite)
{
    if (!overwrite && _trackedEvents.contains(event))
        return;
    _trackedEvents[event].set(ms);
}

void EventScheduler::update()
{
    for (unsigned event : _trackedEvents.keys())
    {
        if (_trackedEvents[event].isRinging())
        {
            _trackedEvents.remove(event);
            currentEvents.push(event);
        }
    }
}