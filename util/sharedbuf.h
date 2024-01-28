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

        unsigned users() const;
        uint8_t* data() const;
        unsigned size() const;

        bool allocated() const;

    private:
        struct SharedBufferData
        {
            SpinLock lock;
            unsigned users;
            uint8_t data[];
        };

        uint8_t* _buf;
        unsigned _size;

        void _create();
        void _destroy();
        void _copy(const SharedBuffer& other);
};
