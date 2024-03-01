#pragma once

#include "msg.h"
#include "table.h"
#include "filter.h"
#include "graph.h"
#include "usage.h"
#include "smsg.h"
#include "../util/timer.h"
#include "../util/priorityqueue.h"
#include <unordered_map>

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
            SharedMessage,
            Size
        };

        enum class Priority
        {
            ACKNACK,
            DEATH_NOTE,
            NETWORK_TABLE,
            PING_PONG = NETWORK_TABLE,
            USER
        };

        struct IO
        {
            struct Public
            {
                PacketTranslator in;
                PacketPriorityQueue out;
            } pub;
            UsageMeter usage;
            SpinLock usageLock;
        };

        MinPriorityQueue<GridMessage> messages;

        GridMessageHub(
            unsigned numIO, 
            unsigned broadcast=100, 
            unsigned maxBroadcastBonusIDs=4,
            unsigned ping=1000);

        unsigned id();

        IO::Public& operator[](unsigned io);

        void setLinkSpeed(unsigned branch, unsigned speed);
        void setName(std::string name);

        bool send(GridMessage& msg, uint16_t receiver, unsigned retries);

        void update();

        void listenFor(SharedGridBuffer& sgb);

        NetworkTable::Nodes getTableNodes();
        void getGraph(GridGraph& graph);
        uint64_t totalBytes();
    
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

        struct PingData
        {
            Timer pinger;
            uint64_t start;
            uint8_t id = 0;
        };
        
        Timer _broadcast;
        unsigned _maxBroadcastBonusIDs;

        std::vector<IO> _ios;
        Hash<InboundData> _inbound;

        Mutex _updateLock;
        NetworkTable _table;
        GridGraph _graph;
        IDFilter _graveyard;
        uint16_t _id;

        SpinLock _nodeLock;
        NetworkTable::Node _node;

        SpinLock _msgIDLock;
        uint16_t _msgID = 0;

        Hash<PingData> _pings;
        unsigned _ping;

        Mutex _sharedDataLock;
        std::unordered_map<std::string, SharedGridBuffer*> _sharedData;

        void _newID();
        void _kill(unsigned id);

        int _getBranchByID(unsigned id);
        void _sendBranch(GridPacket& pkt, unsigned branch);
        void _sendBranch(GridMessage& msg, unsigned branch, unsigned receiver, unsigned retries, bool longAckNAcks=true);
};
