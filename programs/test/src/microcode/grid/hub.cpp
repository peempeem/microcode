#include "hub.h"
#include "../util/log.h"
#include <random>

PACK(struct TableData
{
    uint16_t sender;
    uint32_t rand;
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
    AckNAck(unsigned idx, unsigned id, unsigned ackNAck) 
        : idx(idx), id(id), ackNAck(ackNAck) { }
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

GridMessageHub::GridMessageHub(
    unsigned numIO, 
    unsigned broadcast, 
    unsigned maxBroadcastBonusIDs,
    unsigned ping) 
    :   _broadcast(broadcast), 
        _maxBroadcastBonusIDs(maxBroadcastBonusIDs),
        _ping(ping)
{
    _ios = std::vector<IO>(numIO);
    _newID();
    _node.connections.resize(numIO);
}

unsigned GridMessageHub::id()
{
    return _id;
}

GridMessageHub::IO::Public& GridMessageHub::operator[](unsigned io)
{
    return _ios[io].pub;
}

void GridMessageHub::setName(std::string name)
{
    _nodeLock.lock();
    _node.name = name;
    _nodeLock.unlock();
}

void GridMessageHub::setLinkSpeed(unsigned branch, unsigned speed)
{
    _nodeLock.lock();
    _node.connections[branch].linkspeed = speed;
    _nodeLock.unlock();
    _ios[branch].usageLock.lock();
    _ios[branch].usage.setLinkSpeed(speed);
    _ios[branch].usageLock.unlock();
}

bool GridMessageHub::send(
    GridMessage& msg, 
    uint16_t receiver, 
    unsigned retries)
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

        if (path.size() < 2)
            return false;

        int branch = _getBranchByID(path[1]);
        if (branch != -1)
            _sendBranch(msg, branch, receiver, retries);
    }
    return true;
}

void GridMessageHub::update()
{
    _updateLock.lock();
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
            if (!pkt.get().receiver || pkt.get().receiver == _id)
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

                // send long Ack if packet applicable
                else if (pkt.get().longAckNack)
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
                if (pkt.get().longAckNack)
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

            // packet is malformed and has retries
            if (pkt.get().retries)
            {
                // if already in filter send Ack else send NAck
                unsigned filterID = (((unsigned) pkt.get().id) << 16) + pkt.get().idx;
                unsigned toAck = (_inbound[pkt.get().sender].filter.contains(filterID, false)) ? ACK : NACK;
                ackNAcks.emplace_back((unsigned) pkt.get().idx, (unsigned) pkt.get().id, toAck);
            }

            // packet is malformed and does not have retries but has longAckNack
            else
                ackNAcksLong[pkt.get().sender].emplace_back((unsigned) pkt.get().idx, (unsigned) pkt.get().id, NACK);
        }

        // send AckNAcks to packets received on branch if node id is recognized
        if (!ackNAcks.empty())
        {
            uint16_t id = _node.connections[i].node;
            if (id)
            {
                GridMessage msg(
                    (int) NetworkMessages::AckNAck, 
                    (int) Priority::ACKNACK, 
                    sizeof(AckNAckData) + ackNAcks.size() * sizeof(AckNAck));
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
                GridMessage msg(
                    (int) NetworkMessages::AckNAckLong, 
                    (int) Priority::ACKNACK, 
                    sizeof(AckNAckData) + (*it).size() * sizeof(AckNAck));
                AckNAckData* data = (AckNAckData*) msg.data();
                data->len = (*it).size();
                memcpy(data->data, &(*it)[0], (*it).size() * sizeof(AckNAck));
                _sendBranch(msg, branch, it.key(), 1, false);
            }
        }
    }

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
                        
                        if (td->sender == _id && !_table.filter.contains(td->rand, false))
                        {
                            _kill(_id);
                            _newID();
                        }

                        _nodeLock.lock();
                        _node.connections[(*storeIT).branch].node = td->sender;
                        _nodeLock.unlock();

                        uint8_t* ptr = td->table;
                        std::string name;
                        ptr += _table.sgbufs.deserializeName(ptr, name);
                        if (name == _table.sgbufs.name())
                            ptr += _table.sgbufs.deserializeIDS(ptr);
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
                            // TODO: implement recv ack nack stuff
                        }
                        break;
                    }

                    case (int) NetworkMessages::AckNAckLong:
                    {
                        AckNAckData* anad = (AckNAckData*) (*storeIT).msg.data();
                        for (unsigned i = 0; i < anad->len; ++i)
                        {
                            // TODO: implement recv long ack nack stuff
                        }
                        break;
                    }

                    case (int) NetworkMessages::Ping:
                    {
                        GridMessage msg(
                            (int) NetworkMessages::Pong, 
                            (int) Priority::PING_PONG, 
                            (*storeIT).msg.data(), 
                            (*storeIT).msg.len());
                        send(msg, (*storeIT).msg.sender(), (*storeIT).msg.retries());
                        break;
                    }

                    case (int) NetworkMessages::Pong:
                    {
                        PingPongData* ppd = (PingPongData*) (*storeIT).msg.data();
                        PingData* pd = _pings.at((*storeIT).msg.sender());
                        if (!pd || pd->id != ppd->id)
                            break;
                        NetworkTable::Node* n = _table.nodes.at((*storeIT).msg.sender());
                        if (n)
                            n->ping = sysTime() - pd->start;
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

    // update node usage
    unsigned usageIdx = 0;
    _nodeLock.lock();
    for (auto it = _node.connections.begin(); it != _node.connections.end(); ++it)
    {
        _ios[usageIdx].usageLock.lock();
        _ios[usageIdx].usage.update();
        (*it).usage = _ios[usageIdx].usage.usage();
        _ios[usageIdx++].usageLock.unlock();
    }
    _nodeLock.unlock();

    _table.update();
    _graveyard.preen();

    if (_graveyard.contains(_id, false))
        _newID();

    // update shared data
    for (auto it = _sharedData.begin(); it != _sharedData.end(); ++it)
    {
        it->second->lock();
        if (it->second->canWrite(_id))
        {
            GridMessage msg(
                (int) NetworkMessages::SharedMessage, 
                it->second->priority(), 
                it->second->serialSize(_id));
            uint8_t* ptr = msg.data();
            ptr += it->second->serializeName(ptr);
            ptr += it->second->serializeIDS(ptr, _id);

            for (unsigned i = 0; i < _ios.size(); ++i)
                _sendBranch(msg, i, 0, 0);
        }
        it->second->preen();
        it->second->unlock();
    }
    
    // send table broadcast
    if (_broadcast.isRinging())
    {
        _nodeLock.lock();
        _node.arrival = sysTime();
        _node.time = _node.arrival;
        _node.data.memUsage = sysMemUsage();
        _node.data.memUsageExternal = sysMemUsageExternal();
        _table.writeNode(_id, _node);
        _nodeLock.unlock();

        for (unsigned i = 0; i < _maxBroadcastBonusIDs; ++i)
            _table.nextID();
        
        GridMessage msg(
            (int) NetworkMessages::Table, 
            (int) Priority::NETWORK_TABLE, 
            sizeof(TableData) + _table.sgbufs.serialSize());
        TableData* td = (TableData*) msg.data();
        td->sender = _id;
        td->rand = _table.currentRandom;
        uint8_t* ptr = td->table + _table.sgbufs.serializeName(td->table);
        ptr += _table.sgbufs.serializeIDS(ptr);

        for (unsigned i = 0; i < _ios.size(); ++i)
            _sendBranch(msg, i, 0, 0, false);
        
        _graph.representTable(_table);
        _broadcast.reset();
    }

    // start pings
    for (auto it = _table.nodes.begin(); it != _table.nodes.end(); ++it)
    {
        if (!_pings.contains(it.key()))
        {
            PingData& pd = _pings[it.key()];
            pd.pinger.set(_ping);
        }
    }

    // send pings
    for (auto it = _pings.begin(); it != _pings.end(); ++it)
    {
        if (!_table.nodes.contains(it.key()))
            _pings.remove(it);
        else if (it->pinger.isRinging())
        {
            it->start = sysTime();
            PingPongData ppd;
            ppd.id = ++it->id;

            GridMessage msg(
                (int) NetworkMessages::Ping,
                (int) Priority::PING_PONG, 
                (uint8_t*) &ppd,
                sizeof(PingPongData));

            send(msg, it.key(), 1);
            
            it->pinger.reset();
        }
    }
    _updateLock.unlock();
}

void GridMessageHub::listenFor(SharedGridBuffer& sgb)
{
    _sharedDataLock.lock();
    _sharedData[sgb.name()] = &sgb;
    _sharedDataLock.unlock();
}

NetworkTable::Nodes GridMessageHub::getTableNodes()
{
    _updateLock.lock();
    NetworkTable::Nodes nodes = _table.nodes;
    _updateLock.unlock();
    return nodes;
}

void GridMessageHub::getGraph(GridGraph& graph)
{
    _updateLock.lock();
    graph = _graph;
    _updateLock.unlock();
}

uint64_t GridMessageHub::totalBytes()
{
    uint64_t bytes = 0;
    for (IO& io : _ios)
    {
        io.usageLock.lock();
        bytes += io.usage.total();
        io.usageLock.unlock();
    }
    return bytes;
}

void GridMessageHub::_newID()
{
    do
        _id = esp_random();
    while (!_id || _table.nodes.contains(_id) || _graveyard.contains(_id, false));
}

void GridMessageHub::_kill(unsigned id)
{
    if (_graveyard.contains(id))
        return;

    GridMessage msg(
        (int) NetworkMessages::DeathNote, 
        (int) Priority::DEATH_NOTE, 
        sizeof(DeathNoteData));
    ((DeathNoteData*) msg.data())->victim = id;

    for (unsigned i = 0; i < _ios.size(); ++i)
        _sendBranch(msg, i, -1, 0, false);
}

int GridMessageHub::_getBranchByID(unsigned id)
{
    _nodeLock.lock();
    for (unsigned i = 0; i < _node.connections.size(); ++i)
    {
        if (_node.connections[i].node == id)
        {
            _nodeLock.unlock();
            return i;
        }
    }
    _nodeLock.unlock();
    return -1;
}

void GridMessageHub::_sendBranch(GridPacket& pkt, unsigned branch)
{
    // push packet to IO
    (*this)[branch].out.lock();
    (*this)[branch].out.push(pkt);
    (*this)[branch].out.unlock();

    // update usage
    _ios[branch].usageLock.lock();
    _ios[branch].usage.trackBytes(pkt.size());
    _ios[branch].usageLock.unlock();

    // TODO: add safety buffering
    if (pkt.get().retries)
    {

    }
}

void GridMessageHub::_sendBranch(
    GridMessage& msg, 
    unsigned branch, 
    unsigned receiver, 
    unsigned retries, 
    bool longAckNAcks)
{
    // split up message into packets
    std::vector<GridPacket> packets;
    _msgIDLock.lock();
    unsigned id = _msgID++;
    _msgIDLock.unlock();
    msg.package(packets, _id, receiver, id, retries, longAckNAcks);

    // push packets to IO
    unsigned bytes = 0;   
    (*this)[branch].out.lock();
    for (GridPacket& pkt : packets)
    {
        (*this)[branch].out.push(pkt);
        bytes += pkt.size();
    }
    (*this)[branch].out.unlock();

    // update usage
    _ios[branch].usageLock.lock();
    _ios[branch].usage.trackBytes(bytes);
    _ios[branch].usageLock.unlock();

    // TODO: add safety buffering
    if (retries)
    {
        if (longAckNAcks)
        {

        }
    }
}
