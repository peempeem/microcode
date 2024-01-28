#pragma once

#include "table.h"

class GridGraph
{
    public:
        class Vertex
        {
            public:
                class Edge
                {
                    public:
                        unsigned id;
                        float weight;
                };

                std::vector<Edge> edges; 
        };

        Hash<Vertex> vertices;

        GridGraph();

        void representTable(NetworkTable& table);
        void path(std::vector<unsigned>& p, unsigned start, unsigned end);
        std::string toString();
        std::string pathToString(std::vector<unsigned>& p);
    
    private:
        struct DijkstraData
        {
            float distance = std::numeric_limits<float>::infinity();
            unsigned parent = -1;
        };
        
        Hash<Hash<DijkstraData>> _data;

        void _extractPath(std::vector<unsigned>& p, unsigned start, unsigned end);
};