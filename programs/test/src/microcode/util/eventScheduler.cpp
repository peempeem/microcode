#include "eventScheduler.h"

EventScheduler::EventScheduler()
{

}

bool EventScheduler::has(unsigned event)
{
    return _trackedEvents.contains(event);
}

bool EventScheduler::isEmpty()
{
    return _trackedEvents.empty() && currentEvents.empty();
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
        if (it->isRinging())
        {
            currentEvents.push(it.key());
            _trackedEvents.remove(it);
        }
    }
    _trackedEvents.shrink();
}