#include "graph.h"
#include "../util/priorityqueue.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

//
//// GridGraph Class
//

void GridGraph::representTable(NetworkTable& table)
{
    writeLock();
    _vertices = std::vector<Vertex>(table.nodes.size());
    _toIDX = StaticHash<uint16_t>(table.nodes.size());
    _toID = std::vector<uint16_t>(table.nodes.size());
    
    unsigned i = 0;
    for (auto it = table.nodes.begin(); it != table.nodes.end(); ++it, ++i)
    {
        _toIDX[it.key()] = i;
        _toID[i] = it.key();
    }
    
    i = 0;
    for (auto it = table.nodes.begin(); it != table.nodes.end(); ++it, ++i)
    {
        Vertex& v = _vertices[i];
        v.edges.resize((*it).connections.size());
        for (unsigned j = 0; j < (*it).connections.size(); ++j)
        {
            auto& conn = (*it).connections[j];
            uint16_t* id = _toIDX.at(conn.node);
            if (id)
            {
                v.edges[j].id = *id;
                v.edges[j].weight = 10000 * (conn.usage * 8 + conn.linkspeed) / (float) conn.linkspeed;
            }
            else
                v.edges[j].id = -1;
        }
    }
    _data.clear();
    writeUnlock();
}

void GridGraph::path(std::vector<uint16_t>& p, uint16_t start, uint16_t end)
{
    readLock();
    if (!_toIDX.contains(start) || !_toIDX.contains(end))
    {
        readUnlock();
        return;
    }

    if (start == end)
    {
        p.push_back(start);
        readUnlock();
        return;
    }

    start = _toIDX[start];
    end = _toIDX[end];

    if (!_data.contains(start))
    {
        readUnlock();
        writeLock();
        if (_data.contains(start))
        {
            _extractPath(p, start, end, _data[start]);
            writeUnlock();
        }

        std::vector<DijkstraData>& data = _data[start];
        data.resize(_vertices.size());
        MinPriorityQueue<unsigned> pq;

        pq.push(start, 0);
        data[start].distance = 0;

        while (!pq.empty())
        {
            unsigned v = pq.top();
            pq.pop();

            for (Vertex::Edge& e : _vertices[v].edges)
            {
                if (e.id != (uint16_t) -1 && data[e.id].distance > data[v].distance + e.weight)
                {
                    data[e.id].distance = data[v].distance + e.weight;
                    data[e.id].parent = v;
                    pq.push(e.id, data[e.id].distance);
                }
            }
        }
        _extractPath(p, start, end, _data[start]);
        writeUnlock();
    }
    else
    {
        _extractPath(p, start, end, _data[start]);
        readUnlock();
    }

}

std::string GridGraph::toString()
{
    readLock();
    std::stringstream ss;
    ss << "Enumerating Nodes ...\n";
    ss << "Vertex:\tEdges:\n";
    for (unsigned i = 0; i < _vertices.size(); ++i)
    {
        ss << std::setw(7) << _toID[i] << "\t";
        for (Vertex::Edge& e : _vertices[i].edges)
        {
            if (e.id < _toID.size())
                ss << "{" << _toID[e.id] << ", " << e.weight << "} ";
            else
                ss << "{None, inf}";                
        }
        ss << "\n";
    }
    ss << "Done Enumerating Nodes";
    readUnlock();
    return ss.str();
}

std::string GridGraph::pathToString(std::vector<uint16_t>& p)
{
    std::stringstream ss;
    ss << "[ ";
    for (unsigned i = 0; i < p.size(); ++i)
    {
        ss << p[i];
        if (i < ((int) p.size()) - 1)
            ss << " -> ";
    }
        
    if (p.empty())
        ss << "empty ]";
    else
        ss << " ]";
    return ss.str();
}

void GridGraph::_extractPath(std::vector<uint16_t>& p, uint16_t start, uint16_t end, std::vector<DijkstraData>& data)
{
    uint16_t parent = end;
    while (true)
    {
        if (parent == (uint16_t) -1)
        {
            p.clear();
            return;
        }

        p.push_back(_toID[parent]);
        
        if (parent == start)
        {
            std::reverse(p.begin(), p.end());
            return;
        }
        parent = data[parent].parent;
    }
}
 