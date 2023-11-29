#include "graph.h"
#include <limits.h>

 Graph::Graph()
 {

 }

 unsigned Graph::newNode()
 {
    nodes.insert(_nodeCount, Node(_nodeCount));
    return _nodeCount++;
 }

 void Graph::connect(unsigned node1, unsigned node2, unsigned weight)
 {
    Node* n1 = nodes.at(node1);
    if (!n1)
        return;
    
    if (!weight)
        weight = 1;

    n1->weights[node2] = weight; 
 }

 void Graph::remove(unsigned node)
 {
    nodes.remove(node);
 }

bool Graph::findPath(unsigned start, unsigned end, std::vector<unsigned>& path)
{
    if (!nodes.contains(start) || !nodes.contains(end))
        return false;
    
    path.clear();

    std::vector<unsigned> distance = std::vector<unsigned>(nodes.size(), -1);
    std::vector<bool> sptSet = std::vector<bool>(nodes.size(), false);
}
