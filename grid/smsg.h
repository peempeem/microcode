#pragma once

#include "../util/hash.h"
#include "../util/mutex.h"
#include "../util/sharedbuf.h"
#include <queue>
#include <string>

class SharedGridBuffer : public Mutex
{
    public:
        struct Extras
        {
            struct Send
            {
                uint64_t time = 0;
            };

            struct Past
            {
                uint64_t arrival;
                uint64_t time;
                SharedBuffer buf;

                Past() {}
                Past(uint64_t& arrival, uint64_t& time, SharedBuffer& buf)
                    : arrival(arrival), time(time), buf(buf) {}
            };

            uint64_t arrival = 0;
            bool readFrom;
            bool writtenTo;
            Send send;
            SharedBuffer buf;
            std::queue<Past> stored;
        };

        Hash<Extras> data;

        SharedGridBuffer();
        SharedGridBuffer(
            std::string name,
            unsigned priority,
            bool firstOnly=true,
            unsigned expire=1e6,
            unsigned retries=-1);

        const std::string& name();
        unsigned priority();
        unsigned retries();

        SharedBuffer& touch(unsigned id);
        
        bool canWrite(unsigned id, bool clear=true);
        bool canRead(unsigned id, bool clear=true);

        unsigned serializeName(uint8_t* ptr);
        unsigned serializeID(uint8_t* ptr, unsigned id);
        unsigned serializeIDS(uint8_t* ptr);

        bool serialSizeID(unsigned id, unsigned& size);
        bool serialSizeIDS(unsigned& size);

        static unsigned deserializeName(uint8_t* ptr, std::string& str);
        unsigned deserializeIDS(uint8_t* ptr);

        void preen();

    private:
        std::string _name;
        unsigned _priority;
        unsigned _expire;
        unsigned _retries;
        bool _firstOnly;
};