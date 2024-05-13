#include "pkt.h"
#include "../util/crypt.h"

//
//// GridPacket Class
//

GridPacket::GridPacket()
{

}

GridPacket::GridPacket(            
            unsigned type,
            unsigned idx,
            unsigned total,
            unsigned retries,
            unsigned len,
            unsigned priority,
            unsigned sender,
            unsigned receiver,
            unsigned id,
            const uint8_t* data,
            unsigned stale) : _buf(sizeof(Packet) + len)
{
    get().type          = type;
    get().idx           = idx;
    get().total         = total;
    get().retries       = retries;
    get().len           = len;
    get().priority      = priority;
    get().sender        = sender;
    get().receiver      = receiver;
    get().id            = id;
    get().dhash         = hash32(data, len);
    get().hhash         = hash32(raw(), GRIDPACKET_HEADER_DATA_SIZE);
    memcpy(get().data, data, len);
    _retries = get().retries;
}

GridPacket::GridPacket(const uint8_t* data, unsigned len, unsigned stale) : _buf(data, len), _stale(stale)
{
    _retries = get().retries;
}

GridPacket::GridPacket(const GridPacket& other)
{
    _buf = other._buf;
    _stale = other._stale;
    _retries = other._retries;
}

bool GridPacket::operator<(const GridPacket& other) const
{
    return get().priority < other.get().priority;
}

GridPacket::Packet& GridPacket::get() const
{
    return (Packet&) *_buf.data();
}

uint8_t* GridPacket::raw()
{
    return _buf.data();
}

unsigned GridPacket::size()
{
    return _buf.size();
}

uint32_t GridPacket::IDIDX()
{
    return (((uint32_t) get().id) << 16) + get().idx;
}

void GridPacket::ring()
{
    _stale.ring();
}

bool GridPacket::isStale()
{
    if (isDead())
        return false;
    
    if (_stale.isRinging())
    {
        _retries--;
        _stale.reset();
        return true;
    }
    return false;
}

bool GridPacket::isDead()
{
    return !_retries;
}

//
//// PacketTranslator Class
//

void PacketTranslator::insert(uint8_t byte)
{
    if (!_foundHead)
    {
        _pbuf.packet.insert(byte);
        if (((GridPacket::Packet*) &_pbuf.packet)->hhash == hash32(_pbuf.packet.data, GRIDPACKET_HEADER_DATA_SIZE))
        {
            if (!((GridPacket::Packet*) &_pbuf.packet)->len)
            {
                GridPacket pkt((uint8_t*) &_pbuf.packet, sizeof(GridPacket::Packet) + ((GridPacket::Packet*) &_pbuf.packet)->len);
                packets.push(pkt, pkt.get().priority);
            }
            else
            {
                _foundHead = true;
                _idx = 0;
            }
        }
    }
    else
    {
        _pbuf.data[_idx++] = byte;
        if (_idx >= ((GridPacket::Packet*) &_pbuf.packet)->len || _idx >= MAX_GRIDPACKET_DATA)
        {
            if (((GridPacket::Packet*) &_pbuf.packet)->dhash == hash32(_pbuf.data, _idx))
            {
                GridPacket pkt((uint8_t*) &_pbuf.packet, sizeof(GridPacket::Packet) + ((GridPacket::Packet*) &_pbuf.packet)->len);
                packets.push(pkt, pkt.get().priority);
            }
            else
            {
                GridPacket pkt((uint8_t*) &_pbuf.packet, sizeof(GridPacket::Packet));
                failed.push(pkt, pkt.get().priority);
            }
            _foundHead = false;
        }
    }
}