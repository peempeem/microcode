#include "../util/fifo.h"
#include "../util/hash.h"
#include "../util/lock.h"
#include "../util/timer.h"
#include "../util/sharedbuf.h"
#include "../util/priorityqueue.h"

#define MAX_GRIDPACKET_SIZE 255

#define GRIDPACKET_TYPE_BITS        8
#define GRIDPACKET_IDX_BITS         10
#define GRIDPACKET_TOTAL_BITS       10
#define GRIDPACKET_RETRIES_BITS     4
#define GRIDPACKET_LEN_BITS         10
#define GRIDPACKET_PRIORITY_BITS    6
#define GRIDPACKET_SENDER_BITS      16
#define GRIDPACKET_RECEIVER_BITS    16
#define GRIDPACKET_ID_BITS          16
#define GRIDPACKET_DHASH_BITS       16
#define GRIDPACKET_HHASH_BITS       16

class GridPacket
{
    public:
        PACK(struct Packet
        {
            uint32_t type       : GRIDPACKET_TYPE_BITS;
            uint32_t idx        : GRIDPACKET_IDX_BITS;
            uint32_t total      : GRIDPACKET_TOTAL_BITS;
            uint32_t retries    : GRIDPACKET_RETRIES_BITS;
            uint32_t len        : GRIDPACKET_LEN_BITS;
            uint32_t priority   : GRIDPACKET_PRIORITY_BITS;
            uint32_t sender     : GRIDPACKET_SENDER_BITS;
            uint32_t receiver   : GRIDPACKET_RECEIVER_BITS;
            uint32_t id         : GRIDPACKET_ID_BITS;
            uint32_t dhash      : GRIDPACKET_DHASH_BITS;
            uint32_t hhash      : GRIDPACKET_HHASH_BITS;
            uint8_t data[];
        });

        GridPacket(
            unsigned type,
            unsigned idx,
            unsigned total,
            unsigned retries,
            unsigned len,
            unsigned priority,
            unsigned sender,
            unsigned receiver,
            unsigned id,
            const uint8_t* data,
            unsigned stale=250);
        GridPacket(const uint8_t* data, unsigned len, unsigned stale=250);

        bool operator<(const GridPacket& other) const;

        Packet& get() const;
        uint8_t* raw();
        unsigned size();

        bool isStale();
        bool isDead();
    
    private:
        SharedBuffer _buf;
        Timer _stale;
        unsigned _retries;
};

const unsigned MAX_GRIDPACKET_DATA = MAX_GRIDPACKET_SIZE - sizeof(GridPacket::Packet);
const unsigned GRIDPACKET_HEADER_DATA_SIZE = sizeof(GridPacket::Packet) - GRIDPACKET_HHASH_BITS / 8;

class PacketPriorityQueue : public MinPriorityQueue<GridPacket>, public SpinLock
{
    public:
        PacketPriorityQueue();

        void push(GridPacket& pkt);
};

class PacketTranslator : public PacketPriorityQueue
{
    public:
        PacketTranslator();

        void insert(uint8_t byte);
    
    private:
        PACK(struct PacketBuffer
        {
            InPlaceFIFOBuffer<sizeof(GridPacket::Packet)> packet;
            uint8_t data[MAX_GRIDPACKET_DATA];
        }) _pbuf;

        unsigned _idx;
        bool _foundHead = false;
};