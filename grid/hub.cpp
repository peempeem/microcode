#include "hub.h"
#include "../util/crypt.h"
#include <algorithm>
#include <sstream>


bool GridMessageHub::PacketPriorityQueue::available()
{
    _lock.lock();
    bool ret = _pkts.size();
    _lock.unlock();
    return ret;
}

void GridMessageHub::PacketPriorityQueue::emplace(uint8_t* data, unsigned len)
{
    _lock.lock();
    _pkts[((GridPacket::Fields::Header*) data)->priority].emplace(data, len);
    _lock.unlock();
}

void GridMessageHub::PacketPriorityQueue::put(GridPacket& pkt)
{
    _lock.lock();
    _pkts[pkt.get().header.priority].push(pkt);
    _lock.unlock();
}

void GridMessageHub::PacketPriorityQueue::pop()
{
    _lock.lock();
    unsigned priority = _nextPriority();
    _pkts[priority].pop();
    if (_pkts[priority].empty())
        _pkts.remove(priority);
    _lock.unlock();
    _priority = -1;
}

GridPacket& GridMessageHub::PacketPriorityQueue::front()
{
    _lock.lock();
    std::queue<GridPacket>& pkts = _pkts[_nextPriority()];
    _lock.unlock();
    return pkts.front();
}

unsigned GridMessageHub::PacketPriorityQueue::_nextPriority()
{
    if (_priority != (unsigned) -1)
        return _priority;
    
    std::vector<unsigned> priorities = _pkts.keys();
    _priority = *std::max_element(priorities.begin(), priorities.end());
}

void GridMessageHub::PacketTranslator::insert(uint8_t byte)
{
    GridPacket::Fields::Header* header = (GridPacket::Fields::Header*) _pbuf.header.data;

    if (!foundHead)
    {
        _pbuf.header.insert(byte);
        if (header->hhash == hash32(_pbuf.header.data, HEADER_DATA_SIZE))
        {
            if (!header->len)
            {
                lock();
                packets.emplace((uint8_t*) &_pbuf.header, sizeof(GridPacket::Fields::Header) + header->len);
                unlock();
            }
            else
            {
                foundHead = true;
                idx = 0;
            }
        }
    }
    else
    {
        _pbuf.data[idx++] = byte;
        if (idx >= header->len || idx >= MAX_PACKET_DATA)
        {
            if (header->dhash == hash32(_pbuf.data, idx))
            {
                lock();
                packets.emplace((uint8_t*) &_pbuf.header, sizeof(GridPacket::Fields::Header) + header->len);
                unlock();
            }
            foundHead = false;
        }
    }
}

GridMessageHub::NetworkTable::NetworkTable(unsigned expire) : _expire(expire)
{

}

GridMessageHub::NetworkTable::NetworkTable(const uint8_t* data, unsigned expire) : _expire(expire)
{
    uint64_t time = sysMicros();

    unsigned len = ((ListHeader*) data)->len;
    unsigned idx = sizeof(ListHeader);
    for (unsigned i = 0; i < len; i++)
    {
        NodeBytes* node = (NodeBytes*) (data + idx);
        Node& n = nodes[node->id];
        n.arrival = time;
        n.id = node->id;
        n.distance = node->distance;
        n.connections = std::vector<uint16_t>(node->len);
        for (unsigned j = 0; j < node->len; j++)
            n.connections[j] = node->connections[j];
        idx += sizeof(NodeBytes) + node->len * sizeof(uint16_t);
    }

    /*len = ((ListHeader*) (data + idx))->len;
    idx += sizeof(ListHeader);
    for (unsigned i = 0; i < len; i++)
    {
        DeathNote* deathNote = (DeathNote*) (data + idx);
        deathNotes.insert(deathNote->victim, *deathNote);
    }*/
}

void GridMessageHub::NetworkTable::update(uint16_t id)
{
    uint64_t time = sysMicros();

    /*for (unsigned key : nodes.keys())
    {
        if (time >= nodes[key].arrival + _expire * 1000)
        {
            DeathNote& deathNote = deathNotes[key];
            deathNote.victim = key;
            deathNote.expire = _expire;
        }
    }

    for (unsigned key : deathNotes.keys())
    {
        nodes.remove(key);
        
        DeathNote& deathNote = deathNotes[key];
        uint64_t dt = (time - deathNote.arrival) / 1000;
        if (dt >= deathNote.expire)
            deathNotes.remove(key);
        else
            deathNote.expire -= dt;
    }*/
}

SharedBuffer GridMessageHub::NetworkTable::serialize()
{
    unsigned len = sizeof(ListHeader);
    for (Node& node : nodes)
        len += sizeof(NodeBytes) + node.connections.size() * sizeof(uint16_t);
    len += sizeof(ListHeader) + sizeof(DeathNoteBytes) * deathNotes.size();
    
    SharedBuffer buf(len);

    ((ListHeader*) buf.data())->len = nodes.size();
    unsigned idx = sizeof(ListHeader);
    for (Node& node : nodes)
    {
        NodeBytes* n = (NodeBytes*) (buf.data() + idx);
        n->timestamp = node.timestamp;
        n->id = node.id;
        n->distance = node.distance;
        n->len = node.connections.size();
        for (unsigned i = 0; i < node.connections.size(); i++)
            n->connections[i] = node.connections[i];
        idx += sizeof(NodeBytes) + node.connections.size() * sizeof(uint16_t);
    }

    /*((ListHeader*) buf.data() + idx)->len = nodes.size();
    idx += sizeof(ListHeader);
    for (DeadNode& deadNode : deadNodes)
    {
        *((DeadNode*) (buf.data() + idx)) = deadNode;
        idx += sizeof(DeadNode);
    }*/
    
    return buf;
}

std::string GridMessageHub::NetworkTable::toString()
{
    std::stringstream ss;
    ss << "Network Table Breakdown:\n";
    ss << "------------------------\n";

    for (Node& node : nodes)
    {
        ss << "| Node " << node.id << ":\n";
        //ss << "| Last Update: " << node.timestamp - node.lastUpdate << "\n";
        ss << "| Distance: " << (unsigned) node.distance << "\n";
        ss << "| Connections:";
        /*for (NetworkTable::NodeConnection& conn : node.connections)
            ss << " (" << conn.id << ")";*/
        ss << "\n\n";
    }
    ss << "------------------------\n";
    return ss.str();
}

GridMessageHub::GridMessageHub(unsigned numIO, float netTableSend, unsigned nodeTimeout) : _netTableSend(netTableSend), _nodeTimeout(nodeTimeout)
{
    _ios = std::vector<IO>(numIO);
    _netMsgs = std::vector<Hash<GridMessage>>(numIO);
    srand(sysMicros());
    _id = rand();
    NetworkTable::Node& node = _netTable.nodes[_id];
    node.timestamp = sysMicros();
    node.id = _id;
}

void GridMessageHub::update()
{
    // std::vector<unsigned> ios(_ios.size());
    // for (unsigned i = 0; i < _ios.size(); i++)
    //     ios[i] = i;
    // std::shuffle(ios.begin(), ios.end(), _rng);

    // PacketPriorityQueue packets;

    // for (unsigned i : ios)
    // {
    //     _ios[i].in.lock();
    //     while (true)
    //     {
    //         GridPacket& pkt = _ios[i].in.packets.front();
    //         if (pkt.get().header.type == NETWORK_TABLE)
    //             _netMsgs[i][pkt.get().header.id].insert(pkt);
    //         else
    //             packets.put(_ios[i].in.packets.front());
    //         _ios[i].in.packets.pop();
    //     }
    //     _ios[i].in.unlock();
    // }

    //_netTable.update();

    {
        NetworkTable::Node& node = _netTable.nodes[_id];
        node.timestamp = sysMicros();
        //node.lastUpdate = node.timestamp;
    }



    if (_netTableSend.isReady())
        Serial.println(_netTable.toString().c_str());
        
}