#pragma once

#include "table.h"

class GridGraph
{
    public:
        std::vector<uint16_t> connections(uint16_t node);
        void representTable(NetworkTable& table);
        void path(std::vector<uint16_t>& p, uint16_t start, uint16_t end);
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
        StaticHash<uint16_t> _toIDX;
        std::vector<uint16_t> _toID;

        Hash<std::vector<DijkstraData>> _data;

        void _extractPath(std::vector<uint16_t>& p, uint16_t start, uint16_t end, std::vector<DijkstraData>& data);
};