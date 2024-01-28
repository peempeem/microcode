#include "pkt.h"
#include "../util/crypt.h"

//
//// GridPacket Class
//

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
            unsigned stale) : _buf(sizeof(Packet) + len), _stale(stale), _retries(retries)
{
    get().type      = type;
    get().idx       = idx;
    get().total     = total;
    get().retries   = retries;
    get().len       = len;
    get().priority  = priority;
    get().sender    = sender;
    get().receiver  = receiver;
    get().id        = id;
    get().dhash     = hash32(data, len);
    get().hhash     = hash32(raw(), GRIDPACKET_HEADER_DATA_SIZE);
    memcpy(get().data, data, len);
}

GridPacket::GridPacket(const uint8_t* data, unsigned len, unsigned stale) : _buf(data, len), _stale(stale)
{
    _retries = get().retries;
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

bool GridPacket::isStale()
{
    bool stale = _stale.isRinging();
    if (stale & !isDead())
    {
        if (_retries)
            _retries--;
        _stale.reset();
    }
    return stale;
}

bool GridPacket::isDead()
{
    return _retries;
}

//
//// PacketPriorityQueue Class
//

PacketPriorityQueue::PacketPriorityQueue() : MinPriorityQueue<GridPacket>(), SpinLock()
{

}

void PacketPriorityQueue::push(GridPacket& pkt)
{
    MinPriorityQueue::push(pkt, pkt.get().priority);
}

//
//// PacketTranslator Class
//

PacketTranslator::PacketTranslator() : PacketPriorityQueue()
{

}

void PacketTranslator::insert(uint8_t byte)
{
    if (!_foundHead)
    {
        _pbuf.packet.insert(byte);
        if (((GridPacket::Packet*) &_pbuf.packet)->hhash == hash32(_pbuf.packet.data, GRIDPACKET_HEADER_DATA_SIZE) & ((1 << GRIDPACKET_HHASH_BITS) - 1))
        {
            if (!((GridPacket::Packet*) &_pbuf.packet)->len)
            {
                GridPacket pkt((uint8_t*) &_pbuf.packet, sizeof(GridPacket::Packet) + ((GridPacket::Packet*) &_pbuf.packet)->len);
                lock();
                push(pkt);
                unlock();
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
            if (((GridPacket::Packet*) &_pbuf.packet)->dhash == hash32(_pbuf.data, _idx) & ((1 << GRIDPACKET_DHASH_BITS) - 1))
            {
                GridPacket pkt((uint8_t*) &_pbuf.packet, sizeof(GridPacket::Packet) + ((GridPacket::Packet*) &_pbuf.packet)->len);
                lock();
                push(pkt);
                unlock();
            }
            _foundHead = false;
        }
    }
}