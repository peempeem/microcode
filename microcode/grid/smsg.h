#pragma once

#include "../util/hash.h"
#include "../util/mutex.h"
#include "../util/sharedbuf.h"
#include <string>

class SharedGridBuffer : public Mutex
{
    public:
        struct Extras
        {
            PACK(struct Send
            {
                uint64_t time;
            });

            uint64_t arrival;
            bool read;
            bool written;
            Send send;
            SharedBuffer buf;
        };

        Hash<Extras> data;

        SharedGridBuffer(std::string name, unsigned priority, unsigned expire=1e6);

        const std::string& name();
        unsigned priority();

        SharedBuffer& touch(unsigned id);
        
        bool canWrite(unsigned id);
        bool canRead(unsigned id);

        unsigned serializeName(uint8_t* ptr);
        unsigned serializeIDS(uint8_t* ptr, unsigned id);
        unsigned serializeIDS(uint8_t* ptr);

        unsigned serialSize(unsigned id);
        unsigned serialSize();

        static unsigned deserializeName(uint8_t* ptr, std::string& str);
        unsigned deserializeIDS(uint8_t* ptr);

        void preen();

    private:
        std::string _name;
        unsigned _priority;
        unsigned _expire;
};