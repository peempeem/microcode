#include "hub.h"
#include "../util/log.h"
#include <random>

//
//// GridMessageHub Class
//

GridMessageHub::GridMessageHub(unsigned numIO, unsigned broadcast) : _broadcast(broadcast), _msgID(0)
{
    _ios = std::vector<IO>(numIO);
    _last = sysTime();
    srand(_last);
    _newID();
}

GridMessageHub::IO& GridMessageHub::operator[](unsigned io)
{
    return _ios[io];
}

void GridMessageHub::setLinkSpeed(unsigned branch, unsigned speed)
{
    _node.connections[branch].linkspeed = speed;
}

void GridMessageHub::update()
{
    for (unsigned i = 0; i < _ios.size(); ++i)
    {
        _ios[i].in.lock();
        if (!_ios[i].in.empty())
        {
            GridPacket pkt = _ios[i].in.top();
            _ios[i].in.pop();
            _ios[i].in.unlock();

            if (_inbound[pkt.get().sender].filter.contains((((unsigned) pkt.get().id) << 16) + pkt.get().idx))
                continue;
            
            if (pkt.get().receiver == _id || pkt.get().receiver == (uint16_t) -1)
            {
                InboundData::MessageStore& ms = _inbound[pkt.get().sender].store[pkt.get().id];
                ms.msg.insert(pkt);
                ms.branch = i;
            }
            else
            {
                // find out where to send if not current id
            }
        }
        else
            _ios[i].in.unlock();
    }

    for (auto inIT = _inbound.begin(); inIT != _inbound.end(); ++inIT)
    {
        for (auto storeIT = (*inIT).store.begin(); storeIT != (*inIT).store.end(); ++storeIT)
        {
            if ((*storeIT).msg.received())
            {
                if ((*storeIT).msg.type() >= (int) NetworkMessages::Size)
                {
                    messages.push((*storeIT).msg, (*storeIT).msg.priority());
                    continue;
                }

                switch ((*storeIT).msg.type())
                {
                    case (int) NetworkMessages::Table:
                    {
                        _node.connections[(*storeIT).branch].node = *((uint16_t*) (*storeIT).msg.data());
                        NetworkTable table((*storeIT).msg.data() + sizeof(uint16_t));
                        _table.merge(table);
                        if (_node.connections[(*storeIT).branch].node == _id)
                        {
                            _kill(_id);
                            _newID();
                        }
                        break;
                    }

                    case (int) NetworkMessages::DeathNote:
                    {
                        uint16_t* id = (uint16_t*) (*storeIT).msg.data();
                        if (!_graveyard.contains(*id))
                            _kill(*((uint16_t*) (*storeIT).msg.data()));
                        break;
                    }
                };

                (*inIT).store.remove(storeIT);
            }
            else if ((*storeIT).msg.isDead())
                (*inIT).store.remove(storeIT);
        }
        (*inIT).store.shrink();

        (*inIT).filter.preen();
        if ((*inIT).filter.size() == 0 && (*inIT).store.size() == 0)
            _inbound.remove(inIT);
    }
    _inbound.shrink();

    _node.arrival = sysTime();
    _node.data.time = _node.arrival;

    for (auto it = _node.connections.begin(); it != _node.connections.end(); ++it)
    {
        if ((*it).linkspeed != (unsigned) -1)
        {
            unsigned du = (*it).linkspeed * (_node.arrival - _last) / (float) 1e6;
            (*it).usage = (du >= (*it).usage) ? 0 : (*it).usage - du;
        }
    }

    _last = _node.arrival;
    _table.nodes[_id] = _node;

    _table.update();
    _graveyard.preen();
    
    for (auto it = _table.nodes.begin(); it != _table.nodes.end(); ++it)
    {
        if (_graveyard.contains(it.key(), false))
            _table.nodes.remove(it);
    }
    _table.nodes.shrink();
    
    if (_broadcast.isRinging())
    {
        GridMessage msg((int) NetworkMessages::Table, -1, sizeof(uint16_t) + _table.serialSize());
        *((uint16_t*) msg.data()) = _id;
        _table.serialize(msg.data() + sizeof(uint16_t));

        for (unsigned i = 0; i < _ios.size(); ++i)
            _sendBranch(msg, i, -1, 0);
        
        _graph.representTable(_table);
        _broadcast.reset();
    }
}

void GridMessageHub::_newID()
{
    do
        _id = rand();
    while (_id == (uint16_t) -1 || _table.nodes.contains(_id) || _graveyard.contains(_id));

    _node.arrival = sysTime();
    _node.data.time = _node.arrival;
    _table.nodes[_id] = _node;
}

void GridMessageHub::_kill(unsigned id)
{
    _graveyard.contains(id);
    GridMessage msg((int) NetworkMessages::DeathNote, -1, sizeof(uint16_t));
    *((uint16_t*) msg.data()) = id;
    
    for (unsigned i = 0; i < _ios.size(); ++i)
        _sendBranch(msg, i, -1, 0);
}

void GridMessageHub::_sendBranch(GridMessage& msg, unsigned branch, unsigned receiver, unsigned retries)
{
    std::vector<GridPacket> packets;
    msg.package(packets, _id, receiver, _msgID++, retries);

    unsigned bytes = 0;
    _ios[branch].out.lock();
    for (GridPacket& pkt : packets)
    {
        _ios[branch].out.push(pkt);
        bytes += pkt.size();
    }   
    _ios[branch].out.unlock();

    _bytes += bytes;

    if (_node.connections[branch].linkspeed != (unsigned) -1)
        _node.connections[branch].usage += bytes;
}