#pragma once
#include <stdint.h>

template <unsigned LEN> class InPlaceFIFOBuffer
{
    public:
        uint8_t data[LEN];
        
        void insert(uint8_t byte);
};

#include "fifo.hpp"
