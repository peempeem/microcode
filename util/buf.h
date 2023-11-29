#pragma once
#include <stdint.h>

template <unsigned LEN> class FIFOBuffer
{
    public:
        uint8_t data[LEN];

        FIFOBuffer() {}
        
        void insert(uint8_t byte);
};

template <unsigned LEN> class RingBuffer
{
    public:
        RingBuffer() : _put(0), _get(0) {}

        bool available() { return _put != _get; }
        unsigned size();
        void put(uint8_t byte);
        uint8_t get();
    
    private:
        uint8_t _data[LEN];
        int _put;
        int _get;
};

#include "buf.hpp"
