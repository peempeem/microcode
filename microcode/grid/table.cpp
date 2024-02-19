#include "table.h"

//
//// NetworkTable::Node Class
//

unsigned NetworkTable::Node::serialize(uint8_t* ptr)
{
    uint8_t* end = ptr;
    *((Data*) end) = data;
    end += sizeof(Data);

    *((uint16_t*) end) = connections.size();
    end += sizeof(uint16_t);

    for (auto it = connections.begin(); it != connections.end(); ++it)
    {
        *((uint16_t*) end) = it.key();
        end += sizeof(uint16_t);
        *((Connection*) end) = *it;
        end += sizeof(Connection);
    }

    return end - ptr;
}

unsigned NetworkTable::Node::deserialize(uint8_t* ptr)
{
    uint8_t* end = ptr;
    data = *((Data*) end);
    end += sizeof(Data);

    unsigned numConnections = *((uint16_t*) end);
    end += sizeof(uint16_t);

    for (unsigned i = 0; i < numConnections; i++)
    {
        Connection& conn = connections[*((uint16_t*) end)];
        end += sizeof(uint16_t);
        conn = *((Connection*) end);
        end += sizeof(Connection);
    }

    arrival = sysTime();

    return end - ptr;
}

unsigned NetworkTable::Node::serialSize()
{
    return sizeof(Data) + sizeof(uint16_t) + connections.size() * (sizeof(uint16_t) + sizeof(Connection));
}

//
//// NetworkTable Class
//

NetworkTable::NetworkTable(unsigned expire) : _expire(expire)
{

}

NetworkTable::NetworkTable(uint8_t* ptr, unsigned expire) : _expire(expire)
{
    deserialize(ptr);
}

unsigned NetworkTable::serialize(uint8_t* ptr)
{
    *((uint16_t*) ptr) = nodes.size();
    uint8_t* end = ptr + sizeof(uint16_t);

    for (auto it = nodes.begin(); it != nodes.end(); ++it)
    {
        *((uint16_t*) end) = it.key();
        end += sizeof(uint16_t);
        end += (*it).serialize(end);
    }

    return end - ptr;
}

unsigned NetworkTable::deserialize(uint8_t* ptr)
{
    unsigned numNodes = *((uint16_t*) ptr);
    uint8_t* end = ptr + sizeof(uint16_t);

    for (unsigned i = 0; i < numNodes; i++)
    {
        Node& node = nodes[*((uint16_t*) end)];
        end += sizeof(uint16_t);
        end += node.deserialize(end);
    }
    
    return end - ptr;
}

unsigned NetworkTable::serialSize()
{
    unsigned size = sizeof(uint16_t);
    for (Node& node : nodes)
        size += sizeof(uint16_t) + node.serialSize();
    return size;
}

bool NetworkTable::empty()
{
    return nodes.empty();
}

void NetworkTable::clear()
{
    nodes.clear();
}

void NetworkTable::merge(NetworkTable& table)
{
    for (auto it = table.nodes.begin(); it != table.nodes.end(); ++it)
    {
        Node* node = nodes.at(it.key());
        if (node)
        {
            if (node->data.time < (*it).data.time)
                *node = *it;
        }
        else
            nodes[it.key()] = *it;
    }    
}

void NetworkTable::update()
{
    for (auto it = nodes.begin(); it != nodes.end(); ++it)
    {
        if (sysTime() - (*it).arrival >= _expire)
            nodes.remove(it);
    }
    nodes.shrink();
}

std::string NetworkTable::toString()
{
    std::stringstream ss;
    ss << "Network Table Breakdown:\n";
    ss << "Enumerating Nodes ...\n";
    for (auto it1 = nodes.begin(); it1 != nodes.end(); ++it1)
    {
        ss << "  Node " << it1.key() << ":\n";
        ss << "    arrival: " << (*it1).arrival << "\n";
        ss << "    time: " << (*it1).data.time << "\n";
        ss << "    Connections:";
        if ((*it1).connections.empty())
            ss << " None\n";
        else
        {
            ss << "\n";
            for (auto it2 = (*it1).connections.begin(); it2 != (*it1).connections.end(); ++it2)
            {
                ss << "      Branch " << it2.key() << ":\n";
                ss << "        node: " << (*it2).node << "\n";
                ss << "        linkspeed: " << (*it2).linkspeed << "\n";
                ss << "        usage: " << (*it2).usage << "\n";
            }
        }
    }
    ss << "Done Enumerating Nodes";
    
    return ss.str();
}