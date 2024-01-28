#pragma once

#include "pkt.h"
#include <vector>

class GridMessage
{
    public:
        GridMessage(unsigned death=1000);
        GridMessage(unsigned type, unsigned priority, unsigned len, unsigned death=1000);
        GridMessage(unsigned type, unsigned priority, const uint8_t* data, unsigned len, unsigned death=1000);

        bool operator<(const GridMessage& other) const;

        unsigned type() const;
        unsigned len() const;
        unsigned priority() const;
        unsigned sender() const;
        unsigned id() const;

        uint8_t* data();

        void package(
            std::vector<GridPacket>& packets,
            unsigned sender,
            unsigned receiver,
            unsigned id,
            unsigned retries=-1,
            unsigned stale=10);
        
        bool insert(GridPacket& pkt);
        
        bool received();
        bool isDead() { return _death.isRinging(); }

    private:
        unsigned _type;
        unsigned _len;
        unsigned _priority;
        unsigned _sender;
        unsigned _id;
        SharedBuffer _data;
        std::vector<SharedBuffer> _joiner;
        Timer _death;
        unsigned _numPkts;
};