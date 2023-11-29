#pragma once

#include "timer.h"
#include "hash.h"
#include <queue>

class EventScheduler
{
    public:
        std::queue<unsigned> currentEvents;

        EventScheduler();

        bool has(unsigned event);
        void schedule(unsigned event, unsigned ms, bool overwrite=false);
        void update();
    
    private:
        Hash<Timer> _trackedEvents;
        
};