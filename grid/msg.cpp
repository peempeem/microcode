#include "msg.h"
#include "../util/crypt.h"
#include <string.h>


GridPacket::GridPacket(            
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
            unsigned stale) : _retries(retries), _stale(stale)
{
    _buf = SharedBuffer(sizeof(Fields::Header) + len);
    get().header.type = type;
    get().header.idx = idx;
    get().header.total = total;
    get().header.len = len;
    get().header.retries = retries;
    get().header.priority = priority;
    get().header.sender = sender;
    get().header.receiver = receiver;
    get().header.id = id;
    get().header.dhash = hash32(data, len);
    get().header.hhash = hash32((uint8_t*) &get().header, HEADER_DATA_SIZE);
    memcpy(get().data, data, len);
}

GridPacket::GridPacket(uint8_t* data, unsigned len)
{
    _buf = SharedBuffer(data, len);
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

GridMessage::GridMessage(unsigned death) : _death(death), _len(0)
{
    _death.silence();
}

GridMessage::GridMessage(unsigned type, unsigned priority, const uint8_t* data, unsigned len, unsigned death) : _type(type), _priority(priority), _len(len), _death(death)
{
    _data = SharedBuffer(len);
    memcpy(_data.data(), data, len);
    _death.silence();
}

void GridMessage::package(std::vector<GridPacket>& packets, unsigned sender, unsigned receiver, unsigned id, unsigned retries, unsigned stale)
{
    unsigned numPackets = _len ? (_len + MAX_PACKET_DATA - 1) / MAX_PACKET_DATA : 1;
    packets.reserve(packets.size() + numPackets);

    for (unsigned i = 0; i < numPackets; i++)
    {
        unsigned start = i * MAX_PACKET_DATA;
        unsigned len = _len - start;
        if (len > MAX_PACKET_DATA)
            len = MAX_PACKET_DATA;     
        packets.emplace_back(_type, i, numPackets, len, retries, _priority, sender, receiver, id, data() + start, stale);
    }
}

void GridMessage::insert(GridPacket& pkt)
{
    if (!_joiner.size())
    {
        _type = pkt.get().header.type;
        _joiner = std::vector<SharedBuffer>(pkt.get().header.total);

        if (!pkt.get().header.idx && !pkt.get().header.len)
            _zeroLen = true;
        else
            _zeroLen = false;
    }

    _joiner[pkt.get().header.idx] = SharedBuffer(pkt.get().data, pkt.get().header.len);
    _death.reset();
}

bool GridMessage::received()
{
    if (_data.allocated())
        return true;
    
    if (!_joiner.size())
        return false;
    
    if (!_zeroLen)
    {
        unsigned len = 0;
        for (SharedBuffer& buf : _joiner)
        {
            if (!buf.allocated())
                return false;
            len += buf.size();
        }

        _data = SharedBuffer(len);
        for (unsigned i = 0; i < _joiner.size(); i++)
            memcpy(_data.data() + i * MAX_PACKET_DATA, _joiner[i].data(), _joiner[i].size());
    }
    
    _joiner = std::vector<SharedBuffer>();
    return true;
}




    // std::vector<unsigned> priorities = _rmsgs.keys();
    // std::sort(priorities.begin(), priorities.end());

    // for (unsigned priority : priorities)
    // {
    //     Hash<Hash<GridMessage>>& pmsgs = _rmsgs[priority];

    //     for (unsigned sender : pmsgs.keys())
    //     {
    //         Hash<GridMessage>& smsgs = pmsgs[sender];

    //         for (unsigned id : smsgs.keys())
    //         {
    //             GridMessage& msg = smsgs[id];

    //             if (msg.received())
    //             {
    //                 msgs.push(msg);
    //                 smsgs.remove(id);
    //             }
    //             else if (msg.isDead())
    //                 smsgs.remove(id);
    //         }

    //         if (smsgs.empty())
    //             pmsgs.remove(sender);
    //     }

    //     if (pmsgs.empty())
    //         _rmsgs.remove(priority);
    // }
