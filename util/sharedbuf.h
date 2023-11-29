#pragma once

#include "lock.h"
#include <cstddef>
#include <stdint.h>

class SharedBuffer
{
    public:
        SharedBuffer();
        SharedBuffer(unsigned bytes);
        SharedBuffer(unsigned bytes, uint8_t initializer);
        SharedBuffer(const uint8_t* data, unsigned bytes);
        SharedBuffer(const SharedBuffer& other);
        ~SharedBuffer();

        void operator=(const SharedBuffer& other);

        unsigned users();
        uint8_t* data();
        unsigned size();

        bool allocated() { return _buf != NULL; }

    private:
        struct SharedBufferData
        {
            Lock lock;
            unsigned users;
            uint8_t data[];
        };

        uint8_t* _buf;
        unsigned _size;

        void create();
        void destroy();
        void copy(const SharedBuffer& other);
};
