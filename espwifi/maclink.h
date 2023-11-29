#pragma once

#include "mac.h"
#include "msg.h"
#include <vector>
#include <queue>

class MessageStream : public Lock
{
    public:
        MessageStream(uint8_t type, unsigned len, float hz) : Lock(), _type(type), _buf(len)
        {
            _sendTimer = Timer(1000 / hz);
            _numPkts = (len + MAX_PACKET_DATA - 1) / MAX_PACKET_DATA;
            if (!len)
                _numPkts = 1;
        }

        unsigned id()
        {
            return _id;
        }

        bool ready()
        {
            return _sendTimer.isRinging();
        }

        uint8_t* get()
        {
            return _buf.data();
        }

        Message send(uint16_t id)
        {
            _id = id;
            _sendTimer.reset();
            for (unsigned i = 0; i < _numPkts; i++)
                _sendIndexes.insert(i, i);
            lock();
            Message smsg = Message(_type, _buf.data(), _buf.size());
            unlock();
            return smsg;
        }

        Hash<uint8_t>& sendIndexes()
        {
            return _sendIndexes;
        }
    
    private:
        uint8_t _type;
        int _id;
        unsigned _numPkts;
        SharedBuffer _buf;
        Timer _sendTimer;
        Hash<uint8_t> _sendIndexes;
};

class MACLink 
{
    public:
        std::queue<Message> messages;

        MACLink();
        MACLink(const MAC& mac, unsigned resend=250, unsigned disconnect=1500);

        bool operator==(const MACLink& other);

        void registerMessageStream(MessageStream& mstream);

        void send(Message& msg, uint8_t retries=255, unsigned stale=250);
        void update();

        bool connected();
        int rssi();

        MAC getMAC();

        void _recv_cb(const uint8_t* data, unsigned len, int rssi);
    
    private:
        MAC mac;
        Hash<Packet> sentPackets;
        Hash<Message> recvMessages;
        std::queue<Packet> toProcess;
        std::vector<MessageStream*> mstreams;

        Timer disconnect;
        Timer heartbeat;

        unsigned resend;
        unsigned id = 0;
        int _rssi;

        void _send(Packet& spkt);
        void _send(const uint8_t* data, unsigned len);
};
