#pragma once

#include "../util/sharedbuf.h"
#include "../util/bytestream.h"
#include "../util/fifo.h"
#include "../util/priorityqueue.h"
#include "../util/mutex.h"

class USBMessageBroker
{
    public:
        class Message
        {
            public:
                PACK(struct Fields
                {
                    struct Header
                    {
                        uint8_t type;
                        uint8_t priority;
                        uint16_t len;
                        uint32_t dhash;
                        uint32_t hhash; 
                    } header;
                    uint8_t data[];
                });

                Message();
                Message(
                    uint8_t type, 
                    uint8_t priority, 
                    const uint8_t* data, 
                    uint16_t len);
                Message(Fields::Header& header);

                bool operator<(const Message& other) const;

                Fields& get() const;
                uint8_t* raw();
                unsigned size();

            private:
                SharedBuffer _buf;
        };

        class MessageQueue : public MinPriorityQueue<Message>, public Mutex
        {
            public:
                void push(Message& msg);
        };

        MessageQueue messages;

        void attachStreams(ByteStream* tx, ByteStream* rx);
        void send(uint8_t type, uint8_t priority, uint8_t* data, uint16_t len);
        void update();
    
    private:
        ByteStream* _tx = NULL;
        ByteStream* _rx = NULL;
        InPlaceFIFOBuffer<sizeof(Message::Fields::Header)> _buf;
        Message _rmsg;
        bool _foundHead = false;
        unsigned _idx;

        MessageQueue _smsgs;
        MinPriorityQueue<Message> _rmsgs;

        void _send(uint8_t type, uint8_t priority, uint8_t* data, uint16_t len);
};
