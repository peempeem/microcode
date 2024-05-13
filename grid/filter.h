#pragma once

#include "../util/hash.h"
#include <queue>

class IDFilter
{
    public:
        IDFilter(unsigned timeout=1e6);

        bool contains(unsigned id, bool add=true);
        void remove(unsigned id);
        unsigned size();
        void preen();
    
    private:
        Hash<uint64_t> _timestamps;
        std::queue<unsigned> _next;
        unsigned _timeout;
};
