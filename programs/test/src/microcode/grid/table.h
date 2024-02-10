#pragma once

#include "../hal/hal.h"
#include "../util/hash.h"
#include <random>
#include <sstream>

class NetworkTable
{
    public:
        struct Node
        {   
            PACK(struct Data
            {
                uint64_t time;
            });

            PACK(struct Connection
            {
                uint16_t node = -1;
                uint32_t linkspeed = -1;
                uint32_t usage = 0;
            });

            uint64_t arrival;
            Data data;
            Hash<Connection> connections;
            
            unsigned serialize(uint8_t* ptr);
            unsigned deserialize(uint8_t* ptr);
            unsigned serialSize();
        };

        Hash<Node> nodes;

        NetworkTable(unsigned expire=1e6);
        NetworkTable(uint8_t* ptr, unsigned expire=1e6);

        unsigned serialize(uint8_t* ptr);
        unsigned deserialize(uint8_t* ptr);
        unsigned serialSize();

        bool empty();

        void clear();
        void merge(NetworkTable& table);

        void update();

        std::string toString();
    
    private:
        unsigned _expire;
};
