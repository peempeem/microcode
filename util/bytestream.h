#pragma once

#include "mutex.h"
#include <queue>
#include <streambuf>

class ByteStream : protected Mutex
{
    public:
        void put(const uint8_t* buf, unsigned len);
        void put(std::streambuf& buf);
        
        bool isEmpty();
        unsigned get(uint8_t* buf, unsigned max);

    private:
        struct ByteStreamBuffer
        {
            volatile unsigned read = 0;
            unsigned write = 0;
            uint8_t data[1024];
        };

        Mutex _read;
        Mutex _write;
        std::queue<ByteStreamBuffer> _bufs;
};
