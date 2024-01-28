#pragma once

#include "../util/sharedbuf.h"
#include "../util/fifo.h"
#include "../util/priorityqueue.h"

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

                bool operator<(const Message& other) const { return priority() < other.priority(); }

                uint8_t type() const { return ((Fields*) _buf.data())->header.type; }
                uint8_t priority() const { return ((Fields*) _buf.data())->header.priority; }

                uint8_t* raw() const { return _buf.data(); };
                unsigned size() const { return _buf.size(); }

                uint8_t* data() const { return ((Fields*) _buf.data())->data; }
                unsigned dataSize() const { return ((Fields*) _buf.data())->header.len; };

            private:
                SharedBuffer _buf;
        };

        MinPriorityQueue<Message> messages;

        USBMessageBroker(HardwareSerial* serial) : _serial(serial) {}

        void send(uint8_t type, uint8_t priority, uint8_t* data, uint16_t len);
        void update();
    
    private:
        HardwareSerial* _serial;
        InPlaceFIFOBuffer<sizeof(Message::Fields::Header)> _buf;
        Message _rmsg;
        bool _foundHead = false;
        unsigned _idx;

        MinPriorityQueue<Message> _smsgs;
        MinPriorityQueue<Message> _rmsgs;

        void _send(uint8_t type, uint8_t priority, uint8_t* data, uint16_t len);
};
