#pragma once

#include "../util/sharedbuf.h"
#include "../util/priorityqueue.h"
#include "../util/buf.h"

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

                Message() {}
                Message(uint8_t type, uint8_t priority, const uint8_t* data, uint16_t len);
                Message(Fields::Header& header);

                uint8_t type() { return ((Fields*) _buf.data())->header.type; }
                uint8_t priority() { return ((Fields*) _buf.data())->header.priority; }

                uint8_t* raw() { return _buf.data(); };
                unsigned size() { return _buf.size(); }

                uint8_t* data() { return ((Fields*) _buf.data())->data; }
                unsigned dataSize() { return ((Fields*) _buf.data())->header.len; };

            private:
                SharedBuffer _buf;
        };

        PriorityQueue<Message> messages;

        USBMessageBroker(unsigned port) : _port(port) {}

        void begin(unsigned baudrate, int rx=-1, int tx=-1);

        void send(uint8_t type, uint8_t priority, uint8_t* data, uint16_t len);
        void update();
    
    private:
        unsigned _port;
        FIFOBuffer<sizeof(Message::Fields::Header)> _buf;
        Message _rmsg;
        bool _foundHead = false;
        unsigned _idx;

        PriorityQueue<Message> _smsgs;
        PriorityQueue<Message> _rmsgs;

        void _send(uint8_t type, uint8_t priority, uint8_t* data, uint16_t len);
};
