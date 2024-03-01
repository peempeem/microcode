#include "table.h"

//
//// NetworkTable::Nodes Class
//

std::string NetworkTable::Nodes::toString()
{
    std::stringstream ss;
    ss << "Network Table Breakdown:\n";
    ss << "Enumerating Nodes ...\n";
    for (auto it = begin(); it != end(); ++it)
    {
        ss << "  Node " << it.key() << ":\n";
        ss << "    arrival: " << it->arrival << '\n';
        ss << "    time: " << it->time << '\n';
        ss << "    name: " << it->name << '\n';
        ss << "    memUsage: " << it->data.memUsage << '\n';
        ss << "    memUsageExternal: " << it->data.memUsageExternal << '\n';
        ss << "    ping: " << it->ping << " µs\n";
        ss << "    Connections:";
        if (it->connections.empty())
            ss << " None\n";
        else
        {
            ss << '\n';
            for (unsigned i = 0; i < it->connections.size(); ++i)
            {
                ss << "      Branch " << i << ":\n";
                ss << "        node: " << it->connections[i].node << '\n';
                ss << "        linkspeed: " << it->connections[i].linkspeed << '\n';
                ss << "        usage: " << it->connections[i].usage << '\n';
            }
        }
    }
    ss << "Done Enumerating Nodes";
    
    return ss.str();
}

//
//// NetworkTable::Node Class
//

unsigned NetworkTable::Node::serialize(uint8_t* ptr)
{
    uint8_t* end = ptr;

    *end = name.size();
    end++;

    if (!name.empty())
    {
        memcpy(end, &name[0], name.size());
        end += name.size();
    }

    *((Data*) end) = data;
    end += sizeof(Data);

    *end = connections.size();
    end++;

    unsigned max = min((int) connections.size(), 256);
    for (unsigned i = 0; i < max; ++i)
    {
        *((Connection*) end) = connections[i];
        end += sizeof(Connection);
    }

    return end - ptr;
}

unsigned NetworkTable::Node::deserialize(uint8_t* ptr)
{
    uint8_t* end = ptr;

    unsigned nameSize = *end;
    end++;

    name.resize(nameSize);
    if (nameSize)
    {
        memcpy(&name[0], end, nameSize);
        end += nameSize;
    }

    data = *((Data*) end);
    end += sizeof(Data);

    unsigned numConnections = *end;
    end++;

    connections.resize(numConnections);

    for (unsigned i = 0; i < numConnections; ++i)
    {
        connections[i] = *((Connection*) end);
        end += sizeof(Connection);
    }

    return end - ptr;
}

unsigned NetworkTable::Node::serialSize()
{
    return  sizeof(uint8_t) +
            name.size() +
            sizeof(Data) +
            sizeof(uint8_t) + 
            min((int) connections.size(), 256) * sizeof(Connection);
}

//
//// NetworkTable Class
//

NetworkTable::NetworkTable(unsigned priority, unsigned newRandTimer, unsigned expire) : filterTimer(newRandTimer)
{
    sgbufs = SharedGridBuffer("table", priority, expire);
    currentRandom = esp_random();
    filter.contains(currentRandom);
}

void NetworkTable::update()
{
    for (auto it = sgbufs.data.begin(); it != sgbufs.data.end(); ++it)
    {
        if (!sgbufs.canRead(it.key()))
            continue;
        Node& node = nodes[it.key()];
        node.arrival = it->arrival;
        node.time = it->send.time;
        node.deserialize(it->buf.data());
    }
    sgbufs.preen();

    for (auto it = nodes.begin(); it != nodes.end(); ++it)
    {
        if (!sgbufs.data.contains(it.key()))
            nodes.remove(it);
    }
    nodes.shrink();

    if (filterTimer.isRinging())
    {
        currentRandom = esp_random();
        filter.contains(currentRandom);
        filterTimer.reset();
    }
    filter.preen();
}

void NetworkTable::writeNode(unsigned id, Node& node)
{
    nodes[id] = node;
    
    SharedBuffer& buf = sgbufs.touch(id);
    buf.resize(node.serialSize());
    node.serialize(buf.data());
}

unsigned NetworkTable::nextID()
{
    if (_nextID.empty())
    {
        unsigned i = 0;
        std::vector<unsigned> ids = std::vector<unsigned>(sgbufs.data.size());
        for (auto it = sgbufs.data.begin(); it != sgbufs.data.end(); ++it)
            ids[i++] = it.key();

        std::random_shuffle(ids.begin(), ids.end());
        
        for (unsigned id : ids)
            _nextID.push(id);
    }

    if (_nextID.empty())
        return 0;
    
    unsigned id = _nextID.front();
    _nextID.pop();
    SharedGridBuffer::Extras* ex = sgbufs.data.at(id);
    if (ex)
        ex->writtenTo = true;
    return id;
}
