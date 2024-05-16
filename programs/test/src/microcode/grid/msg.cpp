#include "msg.h"
#include "../util/crypt.h"
#include <string.h>

//
//// GridMessage Class
//

GridMessage::GridMessage(unsigned death) : _death(death), _len(0), _priority(0)
{
    _death.silence();
}

GridMessage::GridMessage(unsigned type, unsigned priority, unsigned len, unsigned death) : _type(type), _priority(priority), _len(len), _death(death)
{
    _numPkts = _len ? (_len + MAX_GRIDPACKET_DATA - 1) / MAX_GRIDPACKET_DATA : 1;
    if (_numPkts > 1)
    {
        _data = SharedBuffer(len + sizeof(uint32_t));
        _numPkts = _data.size() ? (_data.size() + MAX_GRIDPACKET_DATA - 1) / MAX_GRIDPACKET_DATA : 1;
    }
    else
        _data = SharedBuffer(len);
    _death.silence();
}

GridMessage::GridMessage(unsigned type, unsigned priority, const uint8_t* data, unsigned len, unsigned death) : _type(type), _priority(priority), _len(len), _death(death)
{
    _numPkts = _len ? (_len + MAX_GRIDPACKET_DATA - 1) / MAX_GRIDPACKET_DATA : 1;
    if (_numPkts > 1)
    {
        _data = SharedBuffer(len + sizeof(uint32_t));
        _numPkts = _data.size() ? (_data.size() + MAX_GRIDPACKET_DATA - 1) / MAX_GRIDPACKET_DATA : 1;
    }
    else
        _data = SharedBuffer(len);
    memcpy(_data.data(), data, len);
    _death.silence();
}

bool GridMessage::operator<(const GridMessage& other) const
{
    return _priority < other.priority();
}

unsigned GridMessage::type() const
{
    return _type;
}

unsigned GridMessage::len() const
{
    return _len;
}

unsigned GridMessage::priority() const
{
    return _priority;
}

unsigned GridMessage::sender() const
{
    return _sender;
}

unsigned GridMessage::id() const
{
    return _id;
}

unsigned GridMessage::retries() const
{
    return _retries;
}

uint8_t* GridMessage::data()
{
    return _data.data();
}

void GridMessage::package(
    std::vector<GridPacket>& packets, 
    unsigned sender, 
    unsigned receiver, 
    unsigned id, 
    unsigned retries, 
    bool longAckNack,
    unsigned stale)
{
    _retries = retries;

    if (_numPkts > 1)
        *((uint32_t*) (_data.data() + _len)) = hash32(_data.data(), _len);
    
    packets.reserve(packets.size() + _numPkts);
    
    unsigned start;
    unsigned len;
    for (unsigned i = 0; i < _numPkts; i++)
    {
        start = i * MAX_GRIDPACKET_DATA;
        if (_numPkts > 1)
            len = _data.size() - start;
        else
            len = _len - start;
        if (len > MAX_GRIDPACKET_DATA)
            len = MAX_GRIDPACKET_DATA;
             
        packets.emplace_back(
            _type, 
            i, 
            _numPkts, 
            retries, 
            longAckNack, 
            len, 
            _priority, 
            sender, 
            receiver, 
            id, 
            data() + start, 
            stale);
    }

    if (_numPkts)
        _retries = packets.back().get().retries;
    else
        _retries = 0;
}

bool GridMessage::insert(GridPacket& pkt)
{
    if (!_joiner.size())
    {
        _type = pkt.get().type;
        _sender = pkt.get().sender;
        _priority = pkt.get().priority;
        _id = pkt.get().id;
        _retries = pkt.get().retries;

        if (pkt.get().total == 1)
        {
            _len = pkt.get().len;
            _data = SharedBuffer(pkt.get().data, pkt.get().len);
            _death.reset();
            return true;
        }
        else
            _joiner = std::vector<SharedBuffer>(pkt.get().total);
    }

    if (pkt.get().type != _type)
        return false;

    if (pkt.get().idx < _joiner.size())
    {
        if (pkt.get().len < MAX_GRIDPACKET_DATA && pkt.get().idx != pkt.get().total - 1)
            return false;
        
        if (pkt.get().priority > _priority)
            _priority = pkt.get().priority;
        
        _joiner[pkt.get().idx] = SharedBuffer(pkt.get().data, pkt.get().len);
        _death.reset();
        return true;
    }

    return false;
}

bool GridMessage::received()
{
    if (_data.allocated())
        return true;
    
    if (_joiner.empty())
        return false;
    
    _len = 0;
    for (SharedBuffer& buf : _joiner)
    {
        if (!buf.allocated())
            return false;
        _len += buf.size();
    }

    _data = SharedBuffer(_len);
    for (unsigned i = 0; i < _joiner.size(); i++)
        memcpy(_data.data() + i * MAX_GRIDPACKET_DATA, _joiner[i].data(), _joiner[i].size());
    _len -= sizeof(uint32_t);
    
    if (*((uint32_t*) (_data.data() + _len)) != hash32(_data.data(), _len))
    {
        _data = SharedBuffer();
        _joiner.clear();
        return false;
    }
    
    _joiner.clear();
    return true;
}
