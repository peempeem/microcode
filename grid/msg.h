#pragma once

#include "../util/sharedbuf.h"
#include "../util/timer.h"
#include "../util/buf.h"
#include "../util/hash.h"
#include "types.h"
#include <vector>
#include <queue>

#define MAX_PACKET_SIZE 255

class GridPacket
{
    public:
        PACK(struct Fields
        {
            typedef struct Header
            {
                uint8_t type;
                uint8_t idx;
                uint8_t total;
                uint8_t len;
                uint8_t retries;
                uint8_t priority;
                uint16_t sender;
                uint16_t receiver;
                uint16_t id;
                uint32_t dhash;
                uint32_t hhash;
            };

            Header header;
            uint8_t data[];
        });

        GridPacket() : _retries(0) { }
        GridPacket(
            uint8_t type,
            uint8_t idx,
            uint8_t total,
            uint8_t len,
            uint8_t retries,
            uint8_t priority,
            uint16_t sender,
            uint16_t receiver,
            uint16_t id,
            const uint8_t* data,
            unsigned stale=250);
        GridPacket(uint8_t* data, unsigned len);

        Fields& get() { return (Fields&) *_buf.data(); }
        uint8_t* raw() { return _buf.data(); }
        unsigned size() { return _buf.size(); }

        bool isStale();
        bool isDead() { return !_retries; }
    
    private:
        SharedBuffer _buf;
        Timer _stale;
        uint8_t _retries;
};

const unsigned MAX_PACKET_DATA = MAX_PACKET_SIZE - sizeof(GridPacket::Fields);
const unsigned HEADER_DATA_SIZE = sizeof(GridPacket::Fields::Header) - sizeof(uint32_t);

class GridMessage
{
    public:
        GridMessage(unsigned death=1000);
        GridMessage(unsigned type, unsigned priority, const uint8_t* data, unsigned len, unsigned death=1000);

        unsigned type() { return _type; }
        unsigned len() { return _len; }
        unsigned priority() { return _priority; }
        unsigned sender() { return _sender; }

        uint8_t* data() { return _data.data(); }

        void package(
            std::vector<GridPacket>& packets,
            unsigned sender,
            unsigned receiver,
            unsigned id,
            unsigned retries=255,
            unsigned stale=250);
        void insert(GridPacket& fields);
        
        bool received();
        bool isDead() { return _death.isRinging(); }

    private:
        unsigned _type;
        unsigned _len;
        unsigned _priority;
        unsigned _sender;
        SharedBuffer _data;
        std::vector<SharedBuffer> _joiner;
        Timer _death;
        bool _zeroLen;
};
