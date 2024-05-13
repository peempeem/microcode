#pragma once

#include "hash.h"

class Graph
{
    class Node
    {
        public:
            unsigned id;
            Hash<unsigned> weights;

            Node() {}
            Node(unsigned id) : id(id) {}
    };

    public:
        Hash<Node> nodes;

        Graph();

        unsigned newNode();

        void connect(unsigned node1, unsigned node2, unsigned weight);
        void remove(unsigned node);

        bool findPath(unsigned start, unsigned end, std::vector<unsigned>& path);
    
    private:
        unsigned _nodeCount = 0;
};