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

PACK(struct AckNAck
{
    uint32_t sender     : GRIDPACKET_SENDER_BITS;
    uint32_t id         : GRIDPACKET_ID_BITS;
    uint16_t idx        : GRIDPACKET_IDX_BITS;
    uint16_t ackNAck    : 1;

    AckNAck() { }
    AckNAck(GridPacket& pkt, unsigned ackNAck)
    {
        sender = pkt.get().sender;
        id = pkt.get().id;
        idx = pkt.get().idx;
        this->ackNAck = ackNAck;
    }

    uint32_t IDIDX()
    {
        return (((uint32_t) id) << 16) + idx;
    }
});

PACK(struct AckNAckData
{
    uint16_t len;
    uint32_t data[];
});

PACK(struct PingPongData
{
    uint16_t id;
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

void GridMessageHub::setLinkSpeed(unsigned branch, unsigned speed)
{
    _nodeLock.lock();
    _node.connections[branch].linkspeed = speed;
    _nodeLock.unlock();
    _ios[branch].usageLock.lock();
    _ios[branch].usage.setLinkSpeed(speed);
    _ios[branch].usageLock.unlock();
}

void GridMessageHub::setName(std::string name)
{
    _nodeLock.lock();
    _node.name = name;
    _nodeLock.unlock();
}

void GridMessageHub::setInputStream(unsigned port, ByteStream& stream)
{
    _ios[port].in = &stream;
}

void GridMessageHub::setOutputStream(unsigned port, ByteStream& stream)
{
    _ios[port].out = &stream;
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

void GridMessageHub::updateTop(unsigned port)
{
    if (_ios[port].in)
    {
        uint8_t buf[256];
        unsigned pulled;
        do
        {
            pulled = _ios[port].in->get(buf, sizeof(buf));
            for (unsigned i = 0; i < pulled; ++i)
                (*this)[port].in.insert(buf[i]);
        } while (pulled);
    }

    // process all incoming packets for port
    Hash<std::vector<AckNAck>> ackNAcksLong;
    std::vector<AckNAck> ackNAcks;
    MinPriorityQueue<PacketProcssingData> processingPackets;

    // process valid incoming packets
    while (true)
    {
        if ((*this)[port].in.packets.empty())
            break;

        GridPacket pkt = (*this)[port].in.packets.top();
        (*this)[port].in.packets.pop();

        // send Ack if packet has retries
        if (pkt.get().retries)
            ackNAcks.emplace_back(pkt, ACK);

        // filter out packet if seen before
        _filterLock.lock();
        bool result = _filters[pkt.get().sender].contains(pkt.IDIDX());
        _filterLock.unlock();
        if (result)
            continue;
        
        // stash packet for processing if it arrived at its destination
        if (!pkt.get().receiver || pkt.get().receiver == _id)
        {
            PacketProcssingData ppd(pkt, port);
            processingPackets.push(ppd, pkt.get().priority);

            // if brodcast message, propagate to other branches
            if (!pkt.get().receiver 
                && pkt.get().type != (int) NetworkMessages::Table
                && pkt.get().type != (int) NetworkMessages::AckNAck)
            {
                for (unsigned j = 0; j < _ios.size(); ++j)
                {
                    if (port != j)
                        _sendBranch(pkt, j);
                }
            }

            // send long Ack if packet applicable
            else if (pkt.get().longAckNack)
                ackNAcksLong[pkt.get().sender].emplace_back(pkt, ACK);
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
                ackNAcksLong[pkt.get().sender].emplace_back(pkt, NACK);
            
            _filterLock.lock();
            _filters[pkt.get().sender].remove(pkt.IDIDX());
            _filterLock.unlock();
        }
    }

    // process invalid incoming packets
    while (true)
    {
        if ((*this)[port].in.failed.empty())
            break;

        GridPacket pkt = (*this)[port].in.failed.top();
        (*this)[port].in.failed.pop();

        // packet is malformed and has retries
        if (pkt.get().retries)
        {
            // if already in filter send Ack else send NAck
            // _filterLock.lock();
            // unsigned toAck = (_filters[pkt.get().sender].contains(pkt.IDIDX(), false)) ? ACK : NACK;
            // _filterLock.unlock();
            ackNAcks.emplace_back(pkt, NACK);
        }

        // packet is malformed and does not have retries but has longAckNack
        else if (pkt.get().longAckNack)
            ackNAcksLong[pkt.get().sender].emplace_back(pkt, NACK);
    }

    // send AckNAcks to packets received on branch if node id is recognized
    if (!ackNAcks.empty())
    {
        GridMessage msg(
            (int) NetworkMessages::AckNAck, 
            (int) Priority::ACKNACK, 
            sizeof(AckNAckData) + ackNAcks.size() * sizeof(AckNAck));
        AckNAckData* data = (AckNAckData*) msg.data();
        data->len = ackNAcks.size();
        memcpy(data->data, &ackNAcks[0], ackNAcks.size() * sizeof(AckNAck));
        _sendBranch(msg, port, 0, 0);
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

    // send processing packets to updateBottom
    _processingPacketLock.lock();
    while (!processingPackets.empty())
    {
        _processingPackets.push(processingPackets.top(), processingPackets.topPriority());
        processingPackets.pop();
    }
    _processingPacketLock.unlock();
}

void GridMessageHub::updateBottom()
{
    _processingPacketLock.lock();
    while (!_processingPackets.empty())
    {
        PacketProcssingData& ppd = _processingPackets.top();
        MessageStore& ms = _inbound[ppd.pkt.get().sender][ppd.pkt.get().id];
        ms.msg.insert(ppd.pkt);
        ms.branch = ppd.branch;
        _processingPackets.pop();
    }
    _processingPacketLock.unlock();

    _filterLock.lock();
    for (auto it = _filters.begin(); it != _filters.end(); ++it)
    {
        if (!it->size())
            _filters.remove(it);
    }
    _filters.shrink();
    _filterLock.unlock();
    
    _tableLock.lock();
    // itterate through all nodes that sent messsages
    for (auto inIT = _inbound.begin(); inIT != _inbound.end(); ++inIT)
    {
        // itterate through all messages currently building
        for (auto storeIT = inIT->begin(); storeIT != inIT->end(); ++storeIT)
        {
            if (storeIT->msg.received())
            {
                switch (storeIT->msg.type())
                {
                    case (int) NetworkMessages::Table:
                    {
                        TableData* td = (TableData*) storeIT->msg.data();
                        
                        if (td->sender == _id && !_table.filter.contains(td->rand, false))
                        {
                            // _kill(_id);
                            // _newID();
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
                        _ios[storeIT->branch].storageLock.lock();
                        for (unsigned i = 0; i < anad->len; ++i)
                        {
                            AckNAck& ana = ((AckNAck*) anad->data)[i];
                            auto* sender = _ios[storeIT->branch].storage.at(ana.sender);
                            if (sender)
                            {
                                if (ana.ackNAck == ACK)
                                {
                                    sender->remove(ana.IDIDX());
                                    if (sender->empty())
                                        _ios[storeIT->branch].storage.remove(ana.sender);
                                    //Log("acknac") << ana.sender << " " << ana.IDIDX() << "ACK";
                                }
                                else
                                {
                                    GridPacket* pkt = sender->at(ana.IDIDX());
                                    if (pkt)
                                        pkt->ring();
                                    //Log("acknac") << ana.sender << " " << ana.IDIDX() << "NACK";
                                }
                            }
                            // else
                            // {
                            //     Log("acknac rm") << ana.sender << " " << ana.IDIDX();
                            // }
                        }
                        _ios[storeIT->branch].storageLock.unlock();
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
                        PingData* pd = _pings.at((*storeIT).msg.sender());
                        if (!pd)
                            break;
                        PingPongData* ppd = (PingPongData*) (*storeIT).msg.data();
                        if (pd->id != ppd->id)
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
                }
                inIT->remove(storeIT);
            }
            else if ((*storeIT).msg.isDead())
                inIT->remove(storeIT);
        }
        inIT->shrink();

        if (inIT->empty())
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
        it->usage = _ios[usageIdx].usage.usage();
        _ios[usageIdx++].usageLock.unlock();
    }
    _nodeLock.unlock();

    for (auto it = _table.sgbufs.data.begin(); it != _table.sgbufs.data.end(); ++it)
    {
        if (_graveyard.contains(it.key(), false))
            _table.sgbufs.data.remove(it);
    }
    _table.sgbufs.data.shrink();

    _table.update();

    if (_graveyard.contains(_id, false))
        _newID();

    // update shared data
    for (auto it = _sharedData.begin(); it != _sharedData.end(); ++it)
    {
        it->second->lock();
        unsigned msgSize;
        if (it->second->serialSizeIDS(msgSize))
        {
            GridMessage msg(
                (int) NetworkMessages::SharedMessage, 
                it->second->priority(), 
                msgSize);
            
            uint8_t* ptr = msg.data();
            ptr += it->second->serializeName(ptr);
            ptr += it->second->serializeIDS(ptr);

            for (unsigned i = 0; i < _ios.size(); ++i)
                _sendBranch(msg, i, 0, it->second->retries());
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
        _node.ping = 0;
        _node.data.memUsage = sysMemUsage();
        _node.data.memUsageExternal = sysMemUsageExternal();
        _table.writeNode(_id, _node);
        _nodeLock.unlock();

        for (unsigned i = 0; i < _maxBroadcastBonusIDs; ++i)
            _table.nextID();
        
        unsigned msgSize;
        if (_table.sgbufs.serialSizeIDS(msgSize))
        {
            msgSize += sizeof(TableData);
            GridMessage msg(
                (int) NetworkMessages::Table, 
                (int) Priority::NETWORK_TABLE, 
                msgSize);
            TableData* td = (TableData*) msg.data();
            td->sender = _id;
            td->rand = _table.currentRandom;
            uint8_t* ptr = td->table + _table.sgbufs.serializeName(td->table);
            ptr += _table.sgbufs.serializeIDS(ptr);

            for (unsigned i = 0; i < _ios.size(); ++i)
                _sendBranch(msg, i, 0, 15);
        }
        
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
        {
            //_kill(it.key());
            _pings.remove(it);
        }
        else if (it.key() == _id)
            _pings.remove(it);
        else if (it->pinger.isRinging())
        {
            PingPongData ppd;
            ppd.id = ++it->id;
            it->start = sysTime();

            GridMessage msg(
                (int) NetworkMessages::Ping,
                (int) Priority::PING_PONG, 
                (uint8_t*) &ppd,
                sizeof(PingPongData));

            send(msg, it.key(), 15);
            it->pinger.reset();
        }
    }

    // check buffers
    unsigned s = 0;
    for (unsigned i = 0; i < _ios.size(); ++i)
    {
        _ios[i].storageLock.lock();
        for (auto it = _ios[i].storage.begin(); it != _ios[i].storage.end(); ++it)
        {
            for (auto pktIT = it->begin(); pktIT != it->end(); ++pktIT)
            {
                if (pktIT->isStale())
                    _sendBranch(*pktIT, i, false);
                if (pktIT->isDead())
                    it->remove(pktIT);
                else
                    s += pktIT->size();
            }
            if (it->empty())
                _ios[i].storage.remove(it);
            else
                it->shrink();
        }
        _ios[i].storage.shrink();
        _ios[i].storageLock.unlock();
    }
    _tableLock.unlock();
}

void GridMessageHub::listenFor(SharedGridBuffer& sgb)
{
    _sharedDataLock.lock();
    _sharedData[sgb.name()] = &sgb;
    _sharedDataLock.unlock();
}

NetworkTable::Nodes GridMessageHub::getTableNodes()
{
    _tableLock.lock();
    NetworkTable::Nodes nodes = _table.nodes;
    _tableLock.unlock();
    return nodes;
}

void GridMessageHub::getGraph(GridGraph& graph)
{
    graph.readLock();
    graph = _graph;
    graph.readUnlock();
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
        _sendBranch(msg, i, 0, 15);
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

void GridMessageHub::_sendBranch(GridPacket& pkt, unsigned branch, bool canBuffer)
{
    // push packet to IO
    if (_ios[branch].out)
        _ios[branch].out->put(pkt.raw(), pkt.size());
    else
    {
        (*this)[branch].outLock.lock();
        (*this)[branch].out.push(pkt, pkt.get().priority);
        (*this)[branch].outLock.unlock();
    }
    
    // update usage
    _ios[branch].usageLock.lock();
    _ios[branch].usage.trackBytes(pkt.size());
    _ios[branch].usageLock.unlock();

    // buffer packet
    if (canBuffer && pkt.get().retries)
    {
        _ios[branch].storageLock.lock();
        _ios[branch].storage[pkt.get().sender][pkt.IDIDX()] = pkt;
        _ios[branch].storageLock.unlock();
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
    if (_ios[branch].out)
    {
        for (GridPacket& pkt : packets)
        {
            _ios[branch].out->put(pkt.raw(), pkt.size());
            bytes += pkt.size();
        }
    }
    else
    {
        (*this)[branch].outLock.lock();
        for (GridPacket& pkt : packets)
        {
            (*this)[branch].out.push(pkt, pkt.get().priority);
            bytes += pkt.size();
        }
        (*this)[branch].outLock.unlock();
    }

    // update usage
    _ios[branch].usageLock.lock();
    _ios[branch].usage.trackBytes(bytes);
    _ios[branch].usageLock.unlock();

    // buffer packet
    if (retries)
    {
        _ios[branch].storageLock.lock();
        for (GridPacket& pkt : packets)
            _ios[branch].storage[pkt.get().sender][pkt.IDIDX()] = pkt;
        _ios[branch].storageLock.unlock();

        // TODO: LONG ACK NACKS
        if (longAckNAcks)
        {

        }
    }
}
