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
    for (auto it = _trackedEvents.begin(); it != _trackedEvents.end(); ++it)
    {
        if ((*it).isRinging())
        {
            _trackedEvents.remove(it.key());
            currentEvents.push(it.key());
        }
    }
}