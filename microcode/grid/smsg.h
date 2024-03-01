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
                uint64_t time = 0;
            });

            uint64_t arrival = 0;
            bool readFrom;
            bool writtenTo;
            Send send;
            SharedBuffer buf;
        };

        Hash<Extras> data;

        SharedGridBuffer();
        SharedGridBuffer(std::string name, unsigned priority, unsigned expire=1e6);

        const std::string& name();
        unsigned priority();

        SharedBuffer& touch(unsigned id);
        
        bool canWrite(unsigned id, bool clear=true);
        bool canRead(unsigned id, bool clear=true);

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