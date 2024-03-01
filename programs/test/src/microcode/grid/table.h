#pragma once

#include "smsg.h"
#include "filter.h"
#include "../util/timer.h"
#include <random>
#include <sstream>

class NetworkTable
{
    public:
        struct Node
        {   
            PACK(struct Data
            {
                float memUsage = -1;
                float memUsageExternal = -1;
            });

            PACK(struct Connection
            {
                uint16_t node = 0;
                uint32_t linkspeed = -1;
                uint32_t usage = -1;
            });

            unsigned ping = -1;

            uint64_t arrival;
            uint64_t time;
            std::string name;
            Data data;
            std::vector<Connection> connections;
            
            unsigned serialize(uint8_t* ptr);
            unsigned deserialize(uint8_t* ptr);
            unsigned serialSize();
        };

        class Nodes : public Hash<Node>
        {
            public:
                std::string toString();
        };

        Nodes nodes;        
        SharedGridBuffer sgbufs;
        Timer filterTimer;
        IDFilter filter;
        unsigned currentRandom;

        NetworkTable(unsigned priority=1, unsigned newRandTimer=200, unsigned expire=1e6);

        void update();
        void writeNode(unsigned id, Node& node);

        unsigned nextID();
    
    private:
        std::queue<unsigned> _nextID;
};
