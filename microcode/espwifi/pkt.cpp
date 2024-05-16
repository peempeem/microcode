#include "pkt.h"
#include <string.h>

Packet::Packet()
{

}

Packet::Packet(uint8_t type, uint8_t datalen, uint8_t idx, uint8_t retries, uint16_t id, uint16_t msglen, const uint8_t* data, unsigned stale) : staleTimer(stale)
{
    buf = SharedBuffer(sizeof(Fields) + datalen);
    Fields& f = get();
    f.type = type;
    f.datalen = datalen;
    f.idx = idx;
    f.retries = retries;
    f.id = id;
    f.msglen = msglen;
    memcpy(f.data, data, datalen);
}

Packet::Packet(Packet::Fields& fields)
{
    buf = SharedBuffer(sizeof(Fields) + fields.datalen);
    get() = fields;
    memcpy(get().data, fields.data, fields.datalen);
}

Packet::Fields& Packet::get()
{
    return (Fields&) *buf.data();
}

bool Packet::isStale()
{
    bool stale = staleTimer.isRinging();
    if (stale & !isDead())
    {
        get().retries--;
        staleTimer.reset();
    }
    return stale;
}

bool Packet::isDead()
{
    return !get().retries;
}
