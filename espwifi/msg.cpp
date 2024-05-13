#include "msg.h"
#include <string.h>

Message::Message(unsigned death) : death(death)
{
    this->death.silence();
}

Message::Message(uint8_t type, const uint8_t* data, uint16_t len) : _type(type), _len(len)
{
    _data = SharedBuffer(len);
    memcpy(this->data(), data, len);
    death.silence();
}

uint8_t Message::type()
{
    return _type;
}

uint16_t Message::len()
{
    return _len;
}

uint8_t* Message::data()
{
    return _data.data();
}

std::vector<Packet> Message::package(uint16_t id, uint8_t retries, unsigned stale)
{
    unsigned numPkts = (_len + MAX_PACKET_DATA - 1) / MAX_PACKET_DATA;
    if (!_len)
        numPkts = 1;
    
    std::vector<Packet> pkts;
    pkts.reserve(numPkts);

    for (unsigned i = 0; i < numPkts; i++)
    {
        unsigned start = i * MAX_PACKET_DATA;
        unsigned datalen = _len - start;
        if (datalen > MAX_PACKET_DATA)
            datalen = MAX_PACKET_DATA;
        pkts.emplace_back(_type + ReservedMsgs::SIZE, datalen, i, retries, id, _len, &data()[start], stale);
    }
    return pkts;
}

void Message::insert(const Packet::Fields& fields)
{
    if (!checker.size())
    {
        _type = fields.type - ReservedMsgs::SIZE;
        _len = fields.msglen;
        _data = SharedBuffer(fields.msglen);
        unsigned numPkts = (_len + MAX_PACKET_DATA - 1) / MAX_PACKET_DATA;
        if (!_len)
            numPkts = 1;
        checker = SharedBuffer(numPkts, false);
    }
    checker.data()[fields.idx] = true;
    memcpy(&_data.data()[fields.idx * MAX_PACKET_DATA], fields.data, fields.datalen);
    death.reset();      
}

bool Message::received()
{
    if (!checker.size())
        return false;
    
    for (unsigned i = 0; i < checker.size(); i++)
    {
        if (!checker.data()[i])
            return false;
    }
    checker = SharedBuffer();
    death.silence();
    return true;
}

bool Message::isDead()
{
    return death.isRinging();
}
