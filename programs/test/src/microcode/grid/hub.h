#pragma once

#include "msg.h"
#include "table.h"
#include "filter.h"
#include "graph.h"
#include "usage.h"
#include "smsg.h"
#include "../util/rwlock.h"
#include "../util/timer.h"
#include "../util/priorityqueue.h"
#include "../util/bytestream.h"
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
                MinPriorityQueue<GridPacket> out;
                Mutex outLock;
            } pub;
            UsageMeter usage;
            SpinLock usageLock;
            ByteStream* in = NULL;
            ByteStream* out = NULL;
            Mutex storageLock;
            Hash<Hash<GridPacket>> storage;
        };

        MinPriorityQueue<GridMessage> messages;

        GridMessageHub(
            unsigned numIO, 
            unsigned broadcast=100, 
            unsigned maxBroadcastBonusIDs=9,
            unsigned ping=1000);

        unsigned id();

        IO::Public& operator[](unsigned io);

        void setLinkSpeed(unsigned branch, unsigned speed);
        void setName(std::string name);
        void setInputStream(unsigned port, ByteStream& stream);
        void setOutputStream(unsigned port, ByteStream& stream);

        bool send(GridMessage& msg, uint16_t receiver, unsigned retries);
        
        void updateTop(unsigned port);
        void updateBottom();

        void listenFor(SharedGridBuffer& sgb);

        NetworkTable::Nodes getTableNodes();
        void getGraph(GridGraph& graph);
        uint64_t totalBytes();
    
    private:
        struct MessageStore
        {
            GridMessage msg;
            unsigned branch;
        };

        struct PacketProcssingData
        {
            GridPacket pkt;
            unsigned branch;

            PacketProcssingData(GridPacket& pkt, unsigned branch) 
                : pkt(pkt), branch(branch) {}
        };

        struct PingData
        {
            Timer pinger;
            uint16_t id = 0;
            uint64_t start;
        };

        Mutex _processingPacketLock;
        MinPriorityQueue<PacketProcssingData> _processingPackets;
    
        Mutex _tableLock;
        NetworkTable _table;

        GridGraph _graph;

        SpinLock _nodeLock;
        NetworkTable::Node _node;

        SpinLock _msgIDLock;
        uint16_t _msgID = 0;

        SpinLock _filterLock;
        Hash<IDFilter> _filters;

        Mutex _sharedDataLock;
        std::unordered_map<std::string, SharedGridBuffer*> _sharedData;

        Mutex _storedPacketsLock;
        Hash<Hash<GridPacket>> _storedPackets;

        Timer _broadcast;
        unsigned _maxBroadcastBonusIDs;
        Hash<Hash<MessageStore>> _inbound;
        std::vector<IO> _ios;

        IDFilter _graveyard = IDFilter((unsigned) 8e6);
        uint16_t _id;
        Hash<PingData> _pings;
        unsigned _ping;

        void _newID();
        void _kill(unsigned id);

        int _getBranchByID(unsigned id);
        void _sendBranch(GridPacket& pkt, unsigned branch, bool canBuffer=true);
        void _sendBranch(GridMessage& msg, unsigned branch, unsigned receiver, unsigned retries, bool longAckNAcks=false);
};
