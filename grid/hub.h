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
            AckNAck,
            DeathNote,
            Inquisition1,
            Inquisition2,
            Ping,
            Pong,
            Table,
            SharedMessage,
            User
        };

        enum class Priority
        {
            ACK_NACK,
            DEATH_NOTE,
            INQUISITION,
            PING_PONG,
            NETWORK_TABLE,
            USER
        };

        MinPriorityQueue<GridMessage> messages;

        GridMessageHub(
            unsigned numIO, 
            unsigned broadcast=100, 
            unsigned maxBroadcastBonusIDs=9,
            unsigned ping=1000);

        unsigned id();

        void setLinkSpeed(unsigned branch, unsigned speed);
        void setName(std::string name);
        void setInputStream(unsigned port, ByteStream& stream);
        void setOutputStream(unsigned port, ByteStream& stream);
        
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
            unsigned port;
        };

        struct PacketProcessingData
        {
            GridPacket pkt;
            unsigned port;

            PacketProcessingData(GridPacket& pkt, unsigned port) 
                : pkt(pkt), port(port) {}
        };

        PACK(struct AckNAck
        {
            uint32_t sender     : GRIDPACKET_SENDER_BITS;
            uint32_t id         : GRIDPACKET_ID_BITS;
            uint16_t idx        : GRIDPACKET_IDX_BITS;
            uint16_t ackNAck    : 1;

            AckNAck() { }
            AckNAck(GridPacket& pkt, unsigned ackNAck)
            {
                sender = pkt.get().sender;
                id = pkt.get().id;
                idx = pkt.get().idx;
                this->ackNAck = ackNAck;
            }

            uint32_t IDIDX()
            {
                return (((uint32_t) id) << 16) + idx;
            }
        });

        struct IO
        {
            std::vector<GridPacket> processingPackets;
            std::vector<AckNAck> ackNAcks;
            UsageMeter usage;
            PacketTranslator inPkts;
            ByteStream* in = NULL;
            ByteStream* out = NULL;
            Hash<Hash<GridPacket>> storage;
            Timer shrink = Timer(100);
            
            SpinLock usageLock;
            Mutex storageLock;
        };

        struct Inquisition
        {
            Timer timer;
            uint16_t target = 0;
            uint32_t randSend;
            uint32_t randResp;
            bool first;
        };

        struct PingData
        {
            Timer pinger;
            uint16_t id = 0;
            uint64_t start;
        };
        
        NetworkTable _table;
        IDFilter _graveyard = IDFilter((unsigned) 8e6);
        GridGraph _graph;
        NetworkTable::Node _node;
        uint16_t _msgID = 0;
        std::vector<PacketProcessingData> _processingPackets;
        std::unordered_map<std::string, SharedGridBuffer*> _sharedData;
        Hash<IDFilter> _filters;
        Hash<Hash<GridPacket>> _storedPackets;
        Hash<Hash<MessageStore>> _inbound;
        std::vector<IO> _ios;
        Timer _broadcast;
        unsigned _maxBroadcastBonusIDs;
        uint16_t _id;
        Hash<PingData> _pings;
        unsigned _ping;
        Timer _shrink = Timer(100);
        Inquisition _inquisition;

        Mutex _tableLock;
        SpinLock _msgIDLock;
        Mutex _processingPacketLock;
        Mutex _sharedDataLock;
        Mutex _filterLock;
        Mutex _storedPacketsLock;

        void _newID();
        void _kill(unsigned id);

        int _getPortByID(unsigned id);
        void _sendPort(GridPacket& pkt, unsigned port, bool canBuffer=true);
        void _sendPort(GridMessage& msg, unsigned port, unsigned receiver, unsigned retries);
};
