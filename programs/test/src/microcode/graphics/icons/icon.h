#pragma once

#include "../drawable.h"
#include "../../util/sharedbuf.h"

class Icon
{
    public:
        struct IconAllocatorData
        {
            const char* path;
            bool async;
            SharedBuffer buf;
        };

        struct IconBufferData
        {
            bool allocated;
            uint8_t data[];
        };

        bool async;
        unsigned priority;

        Icon();
        Icon(const char* path, bool async=true, unsigned priority=2);

        unsigned width();
        unsigned height();

        bool allocate();
        bool allocated();
        void deallocate();

        uint16_t* bitmap();
        uint16_t at(unsigned x, unsigned y);
    
    private:
        const char* _path;
        Dimension2D<unsigned> _dims;
        SharedBuffer _buf;        
};
