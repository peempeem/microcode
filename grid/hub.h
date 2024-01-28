#pragma once

#include "msg.h"
#include "table.h"
#include "filter.h"
#include "graph.h"
#include "../util/timer.h"
#include "../util/priorityqueue.h"

class GridMessageHub
{
    public:
        enum class NetworkMessages
        {
            Table,
            DeathNote,
            Hops,
            Ack,
            NAck,
            Size
        };

        struct IO
        {
            PacketTranslator in;
            PacketPriorityQueue out;
        };

        MinPriorityQueue<GridMessage> messages;

        GridMessageHub(unsigned numIO, unsigned broadcast=500);

        IO& operator[](unsigned io);

        void setLinkSpeed(unsigned branch, unsigned speed);

        void update();

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

        Timer _broadcast;
        Timer _usage;
        std::vector<IO> _ios;
        Hash<InboundData> _inbound;
        IDFilter _graveyard;

        NetworkTable::Node _node;
        uint16_t _msgID;
        uint64_t _last;
        uint64_t _bytes = 0;

        void _newID();
        void _kill(unsigned id);
        void _sendBranch(GridMessage& msg, unsigned branch, unsigned receiver, unsigned retries);
};
