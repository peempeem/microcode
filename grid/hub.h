#pragma once

#include "msg.h"
#include <random>

class GridMessageHub
{
    public:
        class PacketPriorityQueue
        {
            public:
                PacketPriorityQueue() {}

                bool available();

                void emplace(uint8_t* data, unsigned len);
                void put(GridPacket& pkt);
                void pop();
                GridPacket& front();

            private:
                unsigned _priority = -1;
                Lock _lock;
                Hash<std::queue<GridPacket>> _pkts;

                unsigned _nextPriority();
        };

        class PacketTranslator : public Lock
        {
            public:
                std::queue<GridPacket> packets;

                PacketTranslator() : Lock() {}

                void insert(uint8_t byte);
            
            private:
                PACK(struct PacketBuffer
                {
                    FIFOBuffer<sizeof(GridPacket::Fields::Header)> header;
                    uint8_t data[MAX_PACKET_DATA];   
                });

                unsigned idx;
                bool foundHead = false;
                PacketBuffer _pbuf;
        };

        class NetworkTable
        {
            public:
                struct Node
                {
                    uint64_t arrival;
                    uint64_t timestamp;
                    uint32_t history;
                    uint16_t id;
                    uint8_t idx;
                    uint8_t distance;
                    std::vector<uint16_t> connections;
                };

                struct DeathNote
                {
                    uint64_t expire;
                    uint16_t victim;
                    uint16_t killer;
                };

                Hash<Node> nodes;
                Hash<DeathNote> deathNotes;

                NetworkTable(unsigned expire=2000);
                NetworkTable(const uint8_t* data, unsigned expire=2000);

                void update(uint16_t id);

                SharedBuffer serialize();
                std::string toString();

            private:
                PACK(struct ListHeader
                {
                    uint16_t len;
                });

                PACK(struct NodeBytes
                {
                    uint64_t timestamp;
                    uint16_t id;
                    uint8_t distance;
                    uint8_t len;
                    uint16_t connections[];
                });

                PACK(struct DeathNoteBytes
                {
                    uint64_t expire;
                    uint16_t victim;
                });
        
                unsigned _expire;
        };

        struct IO
        {
            PacketTranslator in;
            PacketPriorityQueue out;
        };

        enum MessageTypes
        {
            NETWORK_TABLE,
            DUPLICATE_ID,
            ACK
        };

        GridMessageHub(unsigned numIO, float netTableSend=2, unsigned nodeTimeout=1000);

        void update();
    
    private:
        uint16_t _id;
        unsigned _nodeTimeout;
        std::vector<IO> _ios;
        std::random_device _rng;
        std::vector<Hash<GridMessage>> _netMsgs;
        Hash<Hash<GridMessage>> _msgs;
        NetworkTable _netTable;
        Rate _netTableSend;
};
