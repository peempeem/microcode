#pragma once

#include "table.h"

class GridGraph
{
    public:
        GridGraph();

        void representTable(NetworkTable& table);
        void path(std::vector<uint16_t>& p, uint16_t start, uint16_t end);
        void pathBFS(std::vector<uint16_t>& p, uint16_t start, uint16_t end);
        std::string toString();
        std::string pathToString(std::vector<uint16_t>& p);
    
    private:
        class Vertex
        {
            public:
                class Edge
                {
                    public:
                        uint16_t id;
                        unsigned weight;
                };

                std::vector<Edge> edges; 
        };
        
        struct DijkstraData
        {
            float distance = std::numeric_limits<unsigned>::max();
            uint16_t parent = -1;
        };
        
        std::vector<Vertex> _vertices;
        HashInPlace<uint16_t> _toIDX;
        std::vector<uint16_t> _toID;

        Hash<std::vector<DijkstraData>> _data;
        Hash<std::vector<uint16_t>> _bfsData;

        void _extractPath(std::vector<uint16_t>& p, uint16_t start, uint16_t end, std::vector<DijkstraData>& data);
};