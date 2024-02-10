#pragma once

#include "msg.h"
#include "table.h"
#include "filter.h"
#include "graph.h"
#include "usage.h"
#include "../util/timer.h"
#include "../util/priorityqueue.h"

class GridMessageHub
{
    public:
        enum class NetworkMessages
        {
            Table,
            DeathNote,
            AckNAck,
            AckNAckLong,
            Ping,
            Pong,
            Size
        };

        struct IO
        {
            struct Public
            {
                PacketTranslator in;
                PacketPriorityQueue out;
            } pub;
            UsageMeter usage;
        };

        MinPriorityQueue<GridMessage> messages;

        GridMessageHub(unsigned numIO, unsigned broadcast=250);

        IO::Public& operator[](unsigned io);

        void setLinkSpeed(unsigned branch, unsigned speed);

        bool send(GridMessage& msg, uint16_t receiver, unsigned retries);

        void updateTop(bool onlyIfLock=true);
        void update();

        uint64_t totalBytes();

        NetworkTable _table;
        uint16_t _id;
        GridGraph _graph;
    
    private:
        struct InboundData
        {
            struct MessageStore
            {
                GridMessage msg;
                unsigned branch;
            };

            IDFilter filter;
            Hash<MessageStore> store;
        };

        PACK(struct Connection
        {
            uint16_t node;
            uint8_t branch;
        });

        BinarySemaphore _lock;
        Timer _broadcast;
        std::vector<IO> _ios;
        Hash<InboundData> _inbound;
        IDFilter _graveyard;
        std::queue<unsigned> _nextToBrodcast;

        NetworkTable::Node _node;
        uint16_t _msgID;

        void _newID();
        void _kill(unsigned id);
        int _getBranchByID(unsigned id);
        void _sendBranch(GridPacket& pkt, unsigned branch);
        void _sendBranch(GridMessage& msg, unsigned branch, unsigned receiver, unsigned retries, bool longAckNAcks=true);
};
