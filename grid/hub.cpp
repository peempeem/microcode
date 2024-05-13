#include "hub.h"
#include "../util/log.h"
#include <random>

#define ACK     1
#define NACK    0

PACK(struct AckNAckData
{
    uint16_t len;
    uint32_t data[];
});

PACK(struct DeathNoteData
{
    uint16_t victim;
});

PACK(struct Inquisition1Data
{
    uint16_t node;
    uint16_t sender;
    uint32_t rand;
});

PACK(struct Inquisition2Data
{
    uint16_t node;
    uint16_t sender;
    uint32_t rand;
    uint32_t response;
});

PACK(struct TableData
{
    uint16_t sender;
    uint32_t rand;
    uint8_t table[];
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
    _inquisition.timer.set(ping);
}

unsigned GridMessageHub::id()
{
    return _id;
}

void GridMessageHub::setLinkSpeed(unsigned port, unsigned speed)
{
    _node.connections[port].linkspeed = speed;
    _ios[port].usageLock.lock();
    _ios[port].usage.setLinkSpeed(speed);
    _ios[port].usageLock.unlock();
}

void GridMessageHub::setName(std::string name)
{
    _node.name = name;
}

void GridMessageHub::setInputStream(unsigned port, ByteStream& stream)
{
    _ios[port].in = &stream;
}

void GridMessageHub::setOutputStream(unsigned port, ByteStream& stream)
{
    _ios[port].out = &stream;
}

void GridMessageHub::updateTop(unsigned port)
{
    IO& io = _ios[port];

    if (io.in)
    {
        uint8_t buf[256];
        unsigned pulled;
        do
        {
            pulled = io.in->get(buf, sizeof(buf));
            for (unsigned i = 0; i < pulled; ++i)
                io.inPkts.insert(buf[i]);
        } while (pulled);
    }

    // process valid incoming packets
    while (!io.inPkts.packets.empty())
    {
        GridPacket& pkt = io.inPkts.packets.top();

        // send Ack if packet has retries
        if (pkt.get().retries)
            io.ackNAcks.emplace_back(pkt, ACK);

        // filter out packet if seen before
        _filterLock.lock();
        bool result = _filters[pkt.get().sender].contains(pkt.IDIDX());
        _filterLock.unlock();
        if (result)
        {
            io.inPkts.packets.pop();
            continue;
        }

        io.processingPackets.emplace_back(pkt);

        // if brodcast message, propagate to other ports
        if (!pkt.get().receiver 
            && pkt.get().type != (int) NetworkMessages::Table
            && pkt.get().type != (int) NetworkMessages::AckNAck)
        {
            for (unsigned j = 0; j < _ios.size(); ++j)
            {
                if (port != j)
                    _sendPort(pkt, j);
            }
        }
        io.inPkts.packets.pop();
    }

    // process invalid incoming packets
    while (!io.inPkts.failed.empty())
    {
        GridPacket& pkt = io.inPkts.failed.top();

        // packet data is malformed and has retries
        if (pkt.get().retries)
        {
            // if already in filter send Ack else send NAck
            _filterLock.lock();
            unsigned toAck = (_filters[pkt.get().sender].contains(pkt.IDIDX(), false)) ? ACK : NACK;
            _filterLock.unlock();
            io.ackNAcks.emplace_back(pkt, toAck);
        }
        io.inPkts.failed.pop();
    }

    // send AckNAcks to packets received on port
    if (!io.ackNAcks.empty())
    {
        GridMessage msg(
            (int) NetworkMessages::AckNAck, 
            (int) Priority::ACK_NACK, 
            sizeof(AckNAckData) + io.ackNAcks.size() * sizeof(AckNAck));
        AckNAckData* data = (AckNAckData*) msg.data();
        data->len = io.ackNAcks.size();
        memcpy(data->data, &io.ackNAcks[0], io.ackNAcks.size() * sizeof(AckNAck));
        _sendPort(msg, port, 0, 0);
        io.ackNAcks.clear();
    }

    // send stored packets
    io.storageLock.lock();
    for (auto sender = io.storage.begin(); sender != io.storage.end(); ++sender)
    {
        for (auto pkt = sender->begin(); pkt != sender->end(); ++pkt)
        {
            if (pkt->isStale())
                _sendPort(*pkt, port, false);
            if (pkt->isDead())
                sender->remove(pkt);
        }
    }
    io.storageLock.unlock();

    // send processing packets to updateBottom
    _processingPacketLock.lock();
    for (GridPacket& pkt : io.processingPackets)
        _processingPackets.emplace_back(pkt, port);
    _processingPacketLock.unlock();
    io.processingPackets.clear();

    // reclaim memory periodically
    if (io.shrink.isRinging())
    {
        io.processingPackets.shrink_to_fit();
        io.ackNAcks.shrink_to_fit();
        io.storageLock.lock();
        for (auto it = io.storage.begin(); it != io.storage.end(); ++it)
        {
            if (it->empty())
                io.storage.remove(it);
            else
                it->shrink();
        }
        io.storage.shrink();
        io.storageLock.unlock();
        io.shrink.reset();
    }
}

void GridMessageHub::updateBottom()
{
    std::vector<uint16_t> path;
    _processingPacketLock.lock();
    for (PacketProcessingData& ppd : _processingPackets)
    {
        if (ppd.pkt.get().receiver && ppd.pkt.get().receiver != _id)
        {
            path.clear();
            _graph.path(path, _id, ppd.pkt.get().receiver);

            if (path.size() > 1)
            {
                int port = _getPortByID(path[1]);
                if (port != -1)
                    _sendPort(ppd.pkt, port);
            }
        }
        else
        {
            MessageStore& ms = _inbound[ppd.pkt.get().sender][ppd.pkt.get().id];
            ms.msg.insert(ppd.pkt);
            ms.port = ppd.port;
        }
    }
    _processingPackets.clear();
    _processingPacketLock.unlock();
    
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
                    case (int) NetworkMessages::AckNAck:
                    {
                        AckNAckData* anad = (AckNAckData*) storeIT->msg.data();
                        _ios[storeIT->port].storageLock.lock();
                        for (unsigned i = 0; i < anad->len; ++i)
                        {
                            AckNAck& ana = ((AckNAck*) anad->data)[i];
                            auto* sender = _ios[storeIT->port].storage.at(ana.sender);
                            if (sender)
                            {
                                if (ana.ackNAck == ACK)
                                    sender->remove(ana.IDIDX(), false);
                                else
                                {
                                    GridPacket* pkt = sender->at(ana.IDIDX());
                                    if (pkt)
                                        pkt->ring();
                                }
                            }
                        }
                        _ios[storeIT->port].storageLock.unlock();
                        break;
                    }

                    case (int) NetworkMessages::DeathNote:
                    {
                        DeathNoteData* dnd = (DeathNoteData*) storeIT->msg.data();
                        _graveyard.contains(dnd->victim);
                        break;
                    }

                    case (int) NetworkMessages::Inquisition1:
                    {
                        Inquisition1Data* i1d = (Inquisition1Data*) storeIT->msg.data();
                        if (i1d->node != _id)
                            break;
                                        
                        GridMessage msg(
                            (int) NetworkMessages::Inquisition2,
                            storeIT->msg.priority(),
                            sizeof(Inquisition2Data));
                        
                        ((Inquisition2Data*) msg.data())->node = i1d->node;
                        ((Inquisition2Data*) msg.data())->sender = i1d->sender;
                        ((Inquisition2Data*) msg.data())->rand = i1d->rand;
                        ((Inquisition2Data*) msg.data())->response = esp_random();
                        for (unsigned i = 0; i < _ios.size(); ++i)
                            _sendPort(msg, i, 0, storeIT->msg.retries());
                        break;
                    }

                    case (int) NetworkMessages::Inquisition2:
                    {
                        Inquisition2Data* i2d = (Inquisition2Data*) storeIT->msg.data();
                        if (i2d->node != _inquisition.target 
                            || i2d->rand != _inquisition.randSend 
                            || i2d->sender != _id
                            || _graveyard.contains(i2d->node, false))
                            break;
                        
                        if (_inquisition.first)
                        {
                            _inquisition.randResp = i2d->response;
                            _inquisition.first = false;
                        }
                        else if (_inquisition.randResp != i2d->response)
                        {
                            Log("hub") << "killed " << _inquisition.target << " by inquisition";
                            _kill(_inquisition.target);
                        }
                    }

                    case (int) NetworkMessages::Table:
                    {
                        TableData* td = (TableData*) storeIT->msg.data();
                        
                        // if (td->sender == _id && !_table.filter.contains(td->rand, false))
                        // {
                        //     _kill(_id);
                        //     _newID();
                        // }

                        _node.connections[storeIT->port].node = td->sender;

                        uint8_t* ptr = td->table;
                        std::string name;
                        ptr += _table.sgbufs.deserializeName(ptr, name);
                        if (name == _table.sgbufs.name())
                            ptr += _table.sgbufs.deserializeIDS(ptr);
                        break;
                    }

                    case (int) NetworkMessages::Ping:
                    {
                        path.clear();
                        _graph.path(path, _id, storeIT->msg.sender());
                        
                        if (path.size() > 1)
                        {
                            int port = _getPortByID(path[1]);
                            if (port != -1)
                            {
                                GridMessage msg(
                                    (int) NetworkMessages::Pong, 
                                    (int) Priority::PING_PONG, 
                                    storeIT->msg.data(), 
                                    storeIT->msg.len());
                                _sendPort(msg, port, storeIT->msg.sender(), storeIT->msg.retries());
                            }
                        }                        
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
                        //messages.push((*storeIT).msg, (*storeIT).msg.priority());
                        break;
                    }
                }
                inIT->remove(storeIT);
            }
            else if ((*storeIT).msg.isDead())
                inIT->remove(storeIT);
        }
    }

    // update node usage
    unsigned usageIdx = 0;
    for (auto it = _node.connections.begin(); it != _node.connections.end(); ++it)
    {
        _ios[usageIdx].usageLock.lock();
        _ios[usageIdx].usage.update();
        it->usage = _ios[usageIdx].usage.usage();
        _ios[usageIdx++].usageLock.unlock();
    }

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
                _sendPort(msg, i, 0, it->second->retries());
        }
        it->second->preen();
        it->second->unlock();
    }
    
    // send table broadcast
    if (_broadcast.isRinging())
    {
        _node.arrival = sysTime();
        _node.time = _node.arrival;
        _node.ping = 0;
        _node.data.memUsage = sysMemUsage();
        _node.data.memUsageExternal = sysMemUsageExternal();
        _table.writeNode(_id, _node);

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
                _sendPort(msg, i, 0, -1);
        }
        
        _graph.representTable(_table);
        _broadcast.reset();
    }

    // start pings
    for (auto it = _table.nodes.begin(); it != _table.nodes.end(); ++it)
    {
        if (it.key() != _id && !_pings.contains(it.key()))
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
        else if (it.key() == _id)
            _pings.remove(it);
        else if (it->pinger.isRinging())
        {
            path.clear();
            _graph.path(path, _id, it.key());

            if (path.size() > 1)
            {
                int port = _getPortByID(path[1]);
                if (port != -1)
                {
                    PingPongData ppd;
                    ppd.id = ++it->id;
                    it->start = sysTime();

                    GridMessage msg(
                        (int) NetworkMessages::Ping,
                        (int) Priority::PING_PONG, 
                        (uint8_t*) &ppd,
                        sizeof(PingPongData));
                    
                    _sendPort(msg, port, it.key(), -1);
                }
            }
            it->pinger.reset();
        }
    }

    // send inquisition
    if (_inquisition.timer.isRinging())
    {
        GridMessage msg(
            (int) NetworkMessages::Inquisition1, 
            (int) Priority::INQUISITION, 
            sizeof(Inquisition1Data));
        
        std::vector<unsigned> ids = _table.nodes.keys();
        uint16_t id;
        uint32_t rand;
        do
        {
            rand = esp_random();
            id = ids[rand % ids.size()];
        } while (!id);

        _inquisition.target = id;
        _inquisition.randSend = rand;
        _inquisition.first = true;
        
        ((Inquisition1Data*) msg.data())->node = id;
        ((Inquisition1Data*) msg.data())->sender = _id;
        ((Inquisition1Data*) msg.data())->rand = rand;
        for (unsigned i = 0; i < _ios.size(); ++i)
            _sendPort(msg, i, 0, -1);

        _inquisition.timer.reset();
    }

    _tableLock.unlock();

    if (_shrink.isRinging())
    {
        _filterLock.lock();
        for (auto it = _filters.begin(); it != _filters.end(); ++it)
        {
            it->preen();
            if (!it->size())
                _filters.remove(it);
        }
        _filters.shrink();
        _filterLock.unlock();

        _processingPacketLock.lock();
        _processingPackets.shrink_to_fit();
        _processingPacketLock.unlock();

        for (auto it = _inbound.begin(); it != _inbound.end(); ++it)
        {
            if (it->empty())
                _inbound.remove(it);
            else
                it->shrink();
        }
        _inbound.shrink();

        _graveyard.preen();

        _shrink.reset();
    }
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
    graph = _graph;
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
        _sendPort(msg, i, 0, -1);
}

int GridMessageHub::_getPortByID(unsigned id)
{
    for (unsigned i = 0; i < _node.connections.size(); ++i)
    {
        if (_node.connections[i].node == id)
            return i;
    }
    return -1;
}

void GridMessageHub::_sendPort(GridPacket& pkt, unsigned port, bool canBuffer)
{
    // push packet to IO
    if (_ios[port].out)
        _ios[port].out->put(pkt.raw(), pkt.size());
    else
        return;
    
    // update usage
    _ios[port].usageLock.lock();
    _ios[port].usage.trackBytes(pkt.size());
    _ios[port].usageLock.unlock();

    // buffer packet
    if (canBuffer && pkt.get().retries && _node.connections[port].node)
    {
        _ios[port].storageLock.lock();
        _ios[port].storage[pkt.get().sender][pkt.IDIDX()] = pkt;
        _ios[port].storageLock.unlock();
    }
}

void GridMessageHub::_sendPort(
    GridMessage& msg, 
    unsigned port, 
    unsigned receiver, 
    unsigned retries)
{
    if (!_ios[port].out)
        return;

    // split up message into packets
    std::vector<GridPacket> packets;
    _msgIDLock.lock();
    unsigned id = _msgID++;
    _msgIDLock.unlock();
    msg.package(packets, _id, receiver, id, retries);

    // push packets to IO
    unsigned bytes = 0;   
    for (GridPacket& pkt : packets)
    {
        _ios[port].out->put(pkt.raw(), pkt.size());
        bytes += pkt.size();
    }

    // update usage
    _ios[port].usageLock.lock();
    _ios[port].usage.trackBytes(bytes);
    _ios[port].usageLock.unlock();

    // buffer packet
    if (retries && _node.connections[port].node)
    {
        _ios[port].storageLock.lock();
        for (GridPacket& pkt : packets)
            _ios[port].storage[pkt.get().sender][pkt.IDIDX()] = pkt;
        _ios[port].storageLock.unlock();
    }
}
