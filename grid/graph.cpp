#include "graph.h"
#include "../util/priorityqueue.h"
#include <algorithm>
#include <sstream>

//
//// GridGraph Class
//

GridGraph::GridGraph()
{

}

void GridGraph::representTable(NetworkTable& table)
{
    for (auto it = table.nodes.begin(); it != table.nodes.end(); ++it)
    {
        Vertex& v = vertices[it.key()];
        v.edges.resize((*it).connections.size());
        for (unsigned i = 0; i < (*it).connections.size(); ++i)
        {
            NetworkTable::Node::Connection& conn = (*it).connections[i];
            v.edges[i].id = conn.node;
            v.edges[i].weight = (conn.usage + conn.linkspeed / 8.0f) / (float) conn.linkspeed;
        }
    }
    _data.clear();
}

void GridGraph::path(std::vector<unsigned>& p, unsigned start, unsigned end)
{
    p.clear();
    if (!vertices.contains(start) || !vertices.contains(end))
        return;

    if (start == end)
    {
        p.push_back(start);
        return;
    }

    if (!_data.contains(start))
    {
        Hash<DijkstraData>& data = _data[start];
        data.reserve(vertices.size());
        MinPriorityQueue<unsigned> pq;

        pq.push(start, 0);
        data[start].distance = 0;

        while (!pq.empty())
        {
            unsigned v = pq.top();
            pq.pop();

            for (Vertex::Edge& e : vertices[v].edges)
            {
                if (data[e.id].distance > data[v].distance + e.weight)
                {
                    data[e.id].distance = data[v].distance + e.weight;
                    data[e.id].parent = v;
                    pq.push(e.id, data[e.id].distance);
                }
            }
        }
    }

    _extractPath(p, start, end);
}

std::string GridGraph::toString()
{
    std::stringstream ss;
    ss << "Vertices:\n";
    for (auto it = vertices.begin(); it != vertices.end(); ++it)
    {
        ss << it.key() << "\tEdges: ";
        for (Vertex::Edge& e : (*it).edges)
        {
            ss << "{" << e.id << ", " << e.weight << "} ";
        }
        ss << "\n";
    }
    
    return ss.str();
}

std::string GridGraph::pathToString(std::vector<unsigned>& p)
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

void GridGraph::_extractPath(std::vector<unsigned>& p, unsigned start, unsigned end)
{
    Hash<DijkstraData>& data = _data[start];
    unsigned parent = end;
    while (true)
    {
        if (parent == (unsigned) -1)
        {
            p.clear();
            return;
        }

        p.push_back(parent);
        
        if (parent == start)
        {
            std::reverse(p.begin(), p.end());
            return;
        }
        parent = data[parent].parent;
    }   
}
 