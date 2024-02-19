#pragma once

#include "../util/hash.h"
#include "../util/lock.h"
#include "../util/sharedbuf.h"
#include <string>

class SharedGridBuffer : public BinarySemaphore
{
    public:
        struct Extras
        {
            PACK(struct Send
            {
                uint64_t time = 0;
            });

            uint64_t arrival = 0;
            bool isNew = false;
            Send send;
            SharedBuffer buf;
        };
        
        Hash<Extras> data;

        SharedGridBuffer(std::string name, unsigned expire=1e6);

        const std::string& name();

        SharedBuffer& touch(unsigned id);
        
        bool isNew(unsigned id);

        void serializeName(uint8_t*& ptr);
        void serializeID(uint8_t*& ptr, unsigned id);

        unsigned serialSize(unsigned id);

        static std::string deserializeName(uint8_t*& ptr);
        void deserializeID(uint8_t*& ptr);

        void preen();

    private:
        std::string _name;
        unsigned _expire;
};