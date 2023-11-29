#pragma once

/*
#include "../hal/hal.h"
#include "../util/rate.h"
#include "../util/sharedbuf.h"
#include "../util/buf.h"
#include <queue>

typedef struct USBMSG
{
    typedef struct HEADER
    {
        typedef struct DATA
        {
            unsigned type;
            unsigned len;
            unsigned hash;
        } msghd_t;

        msghd_t     data;
        unsigned    hash;
    } msgh_t;

    msgh_t      header;
    uint8_t     data[];
} msg_t;

class USBMessage
{
    public:
        USBMessage(unsigned type, const uint8_t* data, unsigned len);
        USBMessage(SharedBuffer& buf);

        unsigned type();
        uint8_t* data();
        unsigned dataSize();

        uint8_t* buffer();
        unsigned bufferSize();
    
    private:
        SharedBuffer _buf;
};

class USBMessageBroker
{
    public:
        USBMessageBroker(unsigned port, float ramSendRate=4);

        void init(unsigned baudrate);

        USBMessage& front();
        void pop();
        bool size();

        void send(unsigned type, const uint8_t* data, unsigned len);
        
        void update();
        void handleInbound();
        void handleOutbound();
    
    private:
        unsigned _port;
        Rate _ramSendRate;
        std::queue<USBMessage> _outq;
        std::queue<USBMessage> _inq;
        Lock _outqLock;
        Lock _inqLock;
        FIFOBuffer<USBMSG::HEADER> _fifo;
};

*/