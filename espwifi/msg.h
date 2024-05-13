#pragma once

#include "pkt.h"
#include "../util/hash.h"
#include <list>

enum ReservedMsgs
{
    ack,
    heartbeat,
    SIZE
};

struct AckMsg 
{
    AckMsg(uint16_t id, uint16_t idx) : type(ReservedMsgs::ack), id(id), idx(idx) {}

    uint8_t type;
    uint8_t idx;
    uint16_t id;
};

class Message
{
    public:
        Message(unsigned death=1000);
        Message(uint8_t type, const uint8_t* data, uint16_t len);
        
        uint8_t type();
        uint16_t len();
        uint8_t* data();

        std::vector<Packet> package(uint16_t id, uint8_t retries=255, unsigned stale=250);
        
        void insert(const Packet::Fields& fields);
        bool received();

        bool isDead();

    private:
        uint8_t _type;
        uint16_t _len;
        SharedBuffer _data;
        SharedBuffer checker;
        Timer death;
};
