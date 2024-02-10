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
            unsigned size;
            uint8_t data[];
        };

        uint8_t* _buf;

        void _create(unsigned size);
        void _destroy();
        void _copy(const SharedBuffer& other);
};