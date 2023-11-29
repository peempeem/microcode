#pragma once

#include "../util/sharedbuf.h"
#include "../util/timer.h"

class Packet
{
    public:
        struct Fields
        {
            uint8_t type;
            uint8_t datalen;
            uint8_t idx;
            uint8_t retries;
            uint16_t id;
            uint16_t msglen;
            uint8_t data[];
        };

        Packet();
        Packet(uint8_t type, uint8_t datalen, uint8_t idx, uint8_t retries, uint16_t id, uint16_t msglen, const uint8_t* data, unsigned stale=250);
        Packet(Packet::Fields& fields);

        Fields& get();

        bool isStale();
        bool isDead();
    
    private:
        SharedBuffer buf;
        Timer staleTimer;
};

const unsigned MAX_ESPNOW_PACKET = 250;
const unsigned MAX_PACKET_DATA = MAX_ESPNOW_PACKET - sizeof(Packet::Fields);
