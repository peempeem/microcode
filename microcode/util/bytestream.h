#pragma once

#include "spinlock.h"
#include <queue>
#include <streambuf>

class ByteStream : protected SpinLock
{
    public:
        void put(const uint8_t* buf, unsigned len);
        void put(std::streambuf& buf);
        
        bool isEmpty();
        unsigned get(uint8_t* buf, unsigned max);

    private:
        struct ByteStreamBuffer
        {
            volatile uint16_t read = 0;
            uint16_t write = 0;
            uint8_t data[512];
        };

        std::queue<ByteStreamBuffer> _bufs;
};
