#pragma once

#include "../util/hash.h"
#include <stdint.h>
#include <queue>

class IDFilter
{
    public:
        IDFilter(unsigned timeout=2e6);

        bool contains(unsigned id, bool add=true);
        void preen();
        unsigned size();
    
    private:
        Hash<uint64_t> _timestamps;
        std::queue<unsigned> _next;
        unsigned _timeout;
};