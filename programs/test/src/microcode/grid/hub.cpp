#include "hub.h"
#include "../util/log.h"
#include <random>

PACK(struct TableData
{
    uint16_t sender;
    uint8_t table[];
});

PACK(struct DeathNoteData
{
    uint16_t victim;
});

#define ACK     1
#define NACK    0

struct AckNAck
{
    uint32_t idx        : GRIDPACKET_IDX_BITS;
    uint32_t id         : GRIDPACKET_ID_BITS;
    uint32_t ackNAck    : 1;

    AckNAck() { }
    AckNAck(unsigned idx, unsigned id, unsigned ackNAck) : idx(idx), id(id), ackNAck(ackNAck) { }
};

PACK(struct AckNAckData
{
    uint16_t len;
    uint32_t data[];
});

PACK(struct PingPongData
{
    uint8_t id;
});

//
//// GridMessageHub Class
//

GridMessageHub::GridMessageHub(unsigned numIO, unsigned broadcast) : _broadcast(broadcast), _msgID(0)
{
    _ios = std::vector<IO>(numIO);
    srand(sysTime());
    _newID();
    _node.arrival = sysTime();
    _node.data.time = _node.arrival;
    _table.nodes[_id] = _node;
}

unsigned GridMessageHub::id()
{
    return _id;
}

GridMessageHub::IO::Public& GridMessageHub::operator[](unsigned io)
{
    return _ios[io].pub;
}

void GridMessageHub::setLinkSpeed(unsigned branch, unsigned speed)
{
    _node.connections[branch].linkspeed = speed;
    _ios[branch].usage.setLinkSpeed(speed);
}

bool GridMessageHub::send(GridMessage& msg, uint16_t receiver, unsigned retries)
{
    if (!receiver)
    {
        // send broadcast message
        for (unsigned i = 0; i < _ios.size(); ++i)
            _sendBranch(msg, i, receiver, retries);
    }
    else
    {
        // send targeted message
        std::vector<uint16_t> path;
        _graph.path(path, _id, receiver);

        int branch = _getBranchByID(path[1]);
        if (branch != -1)
            _sendBranch(msg, branch, receiver, retries);
    }
}

void GridMessageHub::updateTop(bool onlyIfLock)
{
    if (onlyIfLock && !_lock.lock(0))
        return;
    
    // process all incoming packets
    Hash<std::vector<AckNAck>> ackNAcksLong;
    for (unsigned i = 0; i < _ios.size(); ++i)
    {
        std::vector<AckNAck> ackNAcks;
        
        // process valid incoming packets
        while (true)
        {
            (*this)[i].in.packets.lock();
            if ((*this)[i].in.packets.empty())
            {
                (*this)[i].in.packets.unlock();
                break;
            }

            GridPacket pkt = (*this)[i].in.packets.top();
            (*this)[i].in.packets.pop();
            (*this)[i].in.packets.unlock();

            // send Ack if packet has retries
            if (pkt.get().retries)
                ackNAcks.emplace_back((unsigned) pkt.get().idx, (unsigned) pkt.get().id, ACK);

            // filter out packet if seen before
            unsigned filterID = (((unsigned) pkt.get().id) << 16) + pkt.get().idx;
            if (_inbound[pkt.get().sender].filter.contains(filterID))
                continue;
            
            // stash packet for processing if it arrived at its destination
            if (pkt.get().receiver == _id || !pkt.get().receiver)
            {
                InboundData::MessageStore& ms = _inbound[pkt.get().sender].store[pkt.get().id];
                ms.msg.insert(pkt);
                ms.branch = i;

                // if brodcast message, propagate to other branches
                if (!pkt.get().receiver && pkt.get().type != (int) NetworkMessages::Table)
                {
                    for (unsigned j = 0; j < _ios.size(); ++j)
                    {
                        if (i != j)
                            _sendBranch(pkt, j);
                    }
                }

                // send long Ack if packet has retries
                else if (pkt.get().retries)
                    ackNAcksLong[pkt.get().sender].emplace_back((unsigned) pkt.get().idx, (unsigned) pkt.get().id, ACK);
            }

            // determine where to send packet if still in transit
            else
            {
                std::vector<uint16_t> path;
                _graph.path(path, _id, pkt.get().receiver);
                
                if (path.size() > 1)
                {
                    int branch = _getBranchByID(path[1]);
                    if (branch != -1)
                    {
                        _sendBranch(pkt, branch);
                        continue;
                    }
                }
                
                // if no path found or node connection missmatch, send NAck to sender
                ackNAcksLong[pkt.get().sender].emplace_back((unsigned) pkt.get().idx, (unsigned) pkt.get().id, NACK);
                _inbound[pkt.get().sender].filter.remove(filterID);
            }
        }

        // process invalid incoming packets
        while (true)
        {
            (*this)[i].in.failed.lock();
            if ((*this)[i].in.failed.empty())
            {
                (*this)[i].in.failed.unlock();
                break;
            }

            GridPacket pkt = (*this)[i].in.failed.top();
            (*this)[i].in.failed.pop();
            (*this)[i].in.failed.unlock();

            // if packet is malformed and does not have retries, then it is lost
            if (!pkt.get().retries)
                continue;

            // if already in filter send Ack else send NAck
            unsigned filterID = (((unsigned) pkt.get().id) << 16) + pkt.get().idx;
            unsigned toAck = (_inbound[pkt.get().sender].filter.contains(filterID, false)) ? ACK : NACK;
            ackNAcks.emplace_back((unsigned) pkt.get().idx, (unsigned) pkt.get().id, toAck);            
        }

        // send AckNAcks to packets received on branch if node id is recognized
        if (!ackNAcks.empty())
        {
            uint16_t id = _node.connections[i].node;
            if (id != (uint16_t) -1)
            {
                GridMessage msg((int) NetworkMessages::AckNAck, 0, sizeof(AckNAckData) + ackNAcks.size() * sizeof(AckNAck));
                AckNAckData* data = (AckNAckData*) msg.data();
                data->len = ackNAcks.size();
                memcpy(data->data, &ackNAcks[0], ackNAcks.size() * sizeof(AckNAck));
                _sendBranch(msg, i, id, 0, false);
            }
        }
    }

    // send long AckNAcks to senders
    std::vector<uint16_t> path;
    for (auto it = ackNAcksLong.begin(); it != ackNAcksLong.end(); ++it)
    {
        path.clear();
        _graph.path(path, _id, it.key());

        if (path.size() > 1)
        {
            int branch = _getBranchByID(path[1]);
            if (branch != -1)
            {
                GridMessage msg((int) NetworkMessages::AckNAckLong, 0, sizeof(AckNAckData) + (*it).size() * sizeof(AckNAck));
                AckNAckData* data = (AckNAckData*) msg.data();
                data->len = (*it).size();
                memcpy(data->data, &(*it)[0], (*it).size() * sizeof(AckNAck));
                _sendBranch(msg, branch, it.key(), 1, false);
            }
        }
    }

    if (onlyIfLock)
        _lock.unlock();
}

void GridMessageHub::update()
{
    _lock.lock();
    updateTop(false);

    // itterate through all nodes that sent messsages
    for (auto inIT = _inbound.begin(); inIT != _inbound.end(); ++inIT)
    {
        // itterate through all messages currently building
        for (auto storeIT = (*inIT).store.begin(); storeIT != (*inIT).store.end(); ++storeIT)
        {
            if ((*storeIT).msg.received())
            {
                switch ((*storeIT).msg.type())
                {
                    case (int) NetworkMessages::Table:
                    {
                        TableData* td = (TableData*) (*storeIT).msg.data();
                        _node.connections[(*storeIT).branch].node = td->sender;
                        NetworkTable table(td->table);
                        _table.merge(table);
                        if (td->sender == _id)
                        {
                            _kill(_id);
                            Log("hub") << "new id by kill";
                            _newID();
                        }
                        break;
                    }

                    case (int) NetworkMessages::DeathNote:
                    {
                        DeathNoteData* dnd = (DeathNoteData*) (*storeIT).msg.data();
                        _graveyard.contains(dnd->victim);
                        break;
                    }

                    case (int) NetworkMessages::AckNAck:
                    {
                        AckNAckData* anad = (AckNAckData*) (*storeIT).msg.data();
                        for (unsigned i = 0; i < anad->len; ++i)
                        {
                            // TODO: implement ack nack stuff
                        }
                        break;
                    }

                    case (int) NetworkMessages::AckNAckLong:
                    {
                        AckNAckData* anad = (AckNAckData*) (*storeIT).msg.data();
                        for (unsigned i = 0; i < anad->len; ++i)
                        {
                            // TODO: implement long ack nack stuff
                        }
                        break;
                    }

                    case (int) NetworkMessages::Ping:
                    {
                        GridMessage msg((int) NetworkMessages::Pong, 0, (*storeIT).msg.data(), (*storeIT).msg.len());
                        *((PingPongData*) msg.data()) = *((PingPongData*) (*storeIT).msg.data());
                        send(msg, (*storeIT).msg.sender(), (*storeIT).msg.retries());
                        break;
                    }

                    case (int) NetworkMessages::Pong:
                    {
                        // TODO: implement pong
                        break;
                    }

                    case (int) NetworkMessages::SharedMessage:
                    {
                        uint8_t* ptr = (*storeIT).msg.data();
                        std::string str;
                        ptr += SharedGridBuffer::deserializeName(ptr, str);
                        auto it = _sharedData.find(str);
                        if (it != _sharedData.end())
                        {
                            it->second->lock();
                            ptr += it->second->deserializeIDS(ptr);
                            it->second->unlock();
                        }
                        break;
                    }

                    default:
                    {
                        messages.push((*storeIT).msg, (*storeIT).msg.priority());
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
        if (!(*inIT).filter.size() && !(*inIT).store.size())
            _inbound.remove(inIT);
    }
    _inbound.shrink();

    // update hub's node in table
    _node.arrival = sysTime();
    _node.data.time = _node.arrival;

    unsigned usageIdx = 0;
    for (auto it = _node.connections.begin(); it != _node.connections.end(); ++it)
        (*it).usage = _ios[usageIdx++].usage.usage();

    _table.nodes[_id] = _node;
    _table.update();

    // remove nodes from table that are in graveyard
    _graveyard.preen();
    for (auto it = _table.nodes.begin(); it != _table.nodes.end(); ++it)
    {
        if (_graveyard.contains(it.key(), false))
            _table.nodes.remove(it);
    }
    _table.nodes.shrink();

    // update shared data
    for (auto it = _sharedData.begin(); it != _sharedData.end(); ++it)
    {
        it->second->lock();
        if (it->second->canWrite(_id))
        {
            unsigned ss = it->second->serialSize(_id);
            GridMessage msg((int) NetworkMessages::SharedMessage, 1, ss);
            uint8_t* ptr = msg.data();
            ptr += it->second->serializeName(ptr);
            ptr += it->second->serializeIDS(ptr, _id);

            it->second->preen();
            it->second->unlock();

            for (unsigned i = 0; i < _ios.size(); ++i)
                _sendBranch(msg, i, 0, 0);
        }
        else
        {
            it->second->preen();
            it->second->unlock();
        }
    }
    
    // send table broadcast
    if (_broadcast.isRinging())
    {
        unsigned ss = _table.serialSize();
        GridMessage msg((int) NetworkMessages::Table, 0, sizeof(uint16_t) + ss);
        *((uint16_t*) msg.data()) = _id;
        unsigned s = _table.serialize(msg.data() + sizeof(uint16_t));

        for (unsigned i = 0; i < _ios.size(); ++i)
            _sendBranch(msg, i, 0, 0);
        
        _graph.representTable(_table);
        _broadcast.reset();
    }
    
    for (unsigned i = 0; i < _ios.size(); ++i)
        _ios[i].usage.update();

    _lock.unlock();
}

void GridMessageHub::listenFor(SharedGridBuffer& sgb)
{
    _lock.lock();
    _sharedData[sgb.name()] = &sgb;
    _lock.unlock();
}

uint64_t GridMessageHub::totalBytes()
{
    uint64_t bytes = 0;
    for (IO& io : _ios)
        bytes += io.usage.total();
    return bytes;
}

void GridMessageHub::_newID()
{
    do
        _id = rand();
    while (!_id || _table.nodes.contains(_id) || _graveyard.contains(_id));
}

void GridMessageHub::_kill(unsigned id)
{
    _graveyard.contains(id);

    GridMessage msg((int) NetworkMessages::DeathNote, 0, sizeof(DeathNoteData));
    ((DeathNoteData*) msg.data())->victim = id;

    for (unsigned i = 0; i < _ios.size(); ++i)
        _sendBranch(msg, i, 0, 0, false);
}

int GridMessageHub::_getBranchByID(unsigned id)
{
    for (auto it = _node.connections.begin(); it != _node.connections.end(); ++it)
    {
        if ((*it).node == id)
            return it.key();
    }
    return -1;
}

void GridMessageHub::_sendBranch(GridPacket& pkt, unsigned branch)
{
    (*this)[branch].out.lock();
    (*this)[branch].out.push(pkt);
    (*this)[branch].out.unlock();
    _ios[branch].usage.trackBytes(pkt.size());

    // TODO: add safety buffering
    if (pkt.get().retries)
    {

    }
}

void GridMessageHub::_sendBranch(GridMessage& msg, unsigned branch, unsigned receiver, unsigned retries, bool longAckNAcks)
{
    std::vector<GridPacket> packets;
    msg.package(packets, _id, receiver, _msgID++, retries);

    unsigned bytes = 0;
    
    (*this)[branch].out.lock();
    for (GridPacket& pkt : packets)
    {
        (*this)[branch].out.push(pkt);
        bytes += pkt.size();
    }
    (*this)[branch].out.unlock();

    _ios[branch].usage.trackBytes(bytes);

    // TODO: add safety buffering
    if (retries)
    {
        if (longAckNAcks)
        {

        }
    }
}
