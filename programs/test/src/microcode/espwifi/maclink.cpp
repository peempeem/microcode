#include "maclink.h"

#if __has_include(<esp_wifi.h>) && __has_include(<esp_wifi.h>)
#include <esp_wifi.h>
#include <esp_now.h>


MACLink::MACLink()
{
    disconnect.ring();
}

MACLink::MACLink(const MAC& mac, unsigned resend, unsigned disconnect) : mac(mac), resend(resend), disconnect(disconnect), heartbeat(disconnect / 2)
{
    this->disconnect.ring();
    heartbeat.ring();
}

bool MACLink::operator==(const MACLink& other)
{
    return mac == other.mac;
}

void MACLink::registerMessageStream(MessageStream& mstream)
{
    mstreams.push_back(&mstream);
}

void MACLink::send(Message& msg, uint8_t retries, unsigned stale)
{
    for (Packet& pkt : msg.package(id++, retries, stale))
    {
        _send(pkt);
        if (!pkt.isDead())
            sentPackets[(((unsigned) pkt.get().id) << 16) + pkt.get().idx] = pkt;
    }
}

void MACLink::update()
{
    while (!toProcess.empty())
    {
        Packet& pkt = toProcess.front();
        Message& msg = recvMessages[pkt.get().id];
        msg.insert(pkt.get());
        if (msg.received())
        {
            messages.push(msg);
            recvMessages.remove(pkt.get().id);
        }
        toProcess.pop();
    }

    for (auto it = recvMessages.begin(); it != recvMessages.end(); ++it)
    {
        if ((*it).isDead())
            recvMessages.remove(it);
    }
    recvMessages.shrink();
    
    for (auto it = sentPackets.begin(); it != sentPackets.end(); ++it)
    {
        if ((*it).isStale())
        {
            _send((*it));
            if ((*it).isDead())
                sentPackets.remove(it);
        }
    }
    sentPackets.shrink();

    for (MessageStream* mstream : mstreams)
    {
        if (mstream->ready())
        {
            for (auto it = mstream->sendIndexes().begin(); it != mstream->sendIndexes().end(); ++it)
            {
                if (!sentPackets.contains((mstream->id() << 16) + (*it)))
                    mstream->sendIndexes().remove(it);
            }

            if (mstream->sendIndexes().empty())
            {
                Message smsg;
                mstream->createMessage(id, smsg);
                send(smsg);
            }
        }
    }

    if (heartbeat.isRinging())
    {
        uint8_t type = ReservedMsgs::heartbeat;
        _send(&type, sizeof(uint8_t));
    }
}

bool MACLink::connected()
{
    return !disconnect.isRinging();
}

int MACLink::rssi()
{
    if (!connected())
        return -100;
    else
        return _rssi;
}

MAC MACLink::getMAC()
{
    return mac;
}

void MACLink::_recv_cb(const uint8_t* data, unsigned len, int rssi)
{
    _rssi = rssi;

    if (!len)
        return;
    
    disconnect.reset();
    
    switch (data[0])
    {
        case ReservedMsgs::ack:
        {
            AckMsg* msg = (AckMsg*) data;
            sentPackets.remove((((unsigned) msg->id) << 16) + msg->idx);
            break;
        }

        case ReservedMsgs::heartbeat:
        {
            break;
        }

        default:
        {
            Packet::Fields* fields = (Packet::Fields*) data;
            if (fields->retries)
            {
                AckMsg msg(fields->id, fields->idx);
                _send((uint8_t*) &msg, sizeof(AckMsg));
            }
            toProcess.emplace(*fields);
            break;
        }
    }
}

void MACLink::_send(Packet& pkt)
{
    _send((const uint8_t*) &pkt.get(), sizeof(Packet::Fields) + pkt.get().datalen);
}

void MACLink::_send(const uint8_t* data, unsigned len)
{
    heartbeat.reset();
    esp_now_send(mac.addr, data, len);
}

#endif

