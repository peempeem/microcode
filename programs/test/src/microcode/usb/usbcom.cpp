#include "usbcom.h"
#include "../util/crypt.h"
#include "../hal/hal.h"

enum ReservedTypes
{
    ping,
    reping,
    setbaud,
    nonimp3
};

USBMessageBroker::Message::Message(uint8_t type, uint8_t priority, const uint8_t* data, uint16_t len)
{
    _buf = SharedBuffer(sizeof(Fields) + len);
    ((Fields*) _buf.data())->header.type = type;
    ((Fields*) _buf.data())->header.priority = priority;
    ((Fields*) _buf.data())->header.len = len;
    ((Fields*) _buf.data())->header.dhash = hash32(data, len);
    ((Fields*) _buf.data())->header.hhash = hash32((const uint8_t*) &((Fields*) _buf.data())->header, sizeof(Fields::Header) - sizeof(Fields::Header::hhash));
    memcpy(((Fields*) _buf.data())->data, data, len);
}

USBMessageBroker::Message::Message(Fields::Header& header)
{
    _buf = SharedBuffer(sizeof(Fields) + header.len);
    ((Fields*) _buf.data())->header = header;
}

void USBMessageBroker::send(uint8_t type, uint8_t priority, uint8_t* data, uint16_t len)
{
    _send(type + sizeof(ReservedTypes), priority + 1, data, len);
}

#include "../util/log.h"

void USBMessageBroker::update()
{
    if (!_tx || !_rx)
        return;
    
    _rx->lock();
    while (!_rx->isEmptyUnlocked())
    {
        uint8_t byte = _rx->getUnlocked();
        if (!_foundHead)
        {
            _buf.insert(byte);
            if (((Message::Fields::Header*) _buf.data)->hhash == hash32(_buf.data, sizeof(Message::Fields::Header) - sizeof(Message::Fields::Header::hhash)))
            {
                _rmsg = Message(*((Message::Fields::Header*) _buf.data));
                _idx = 0;
                if (_rmsg.dataSize() == 0)
                {
                    if (_rmsg.type() < sizeof(ReservedTypes))
                        _rmsgs.push(_rmsg, _rmsg.priority());
                    else
                    {
                        ((Message::Fields*) _rmsg.raw())->header.type -= sizeof(ReservedTypes);
                        messages.push(_rmsg, _rmsg.priority());
                    }
                }
                else
                    _foundHead = true;                
            }
        }
        else
        {
            _rmsg.data()[_idx++] = byte;
            if (_idx >= _rmsg.dataSize())
            {
                if (((Message::Fields*) _rmsg.raw())->header.dhash == hash32(_rmsg.data(), _rmsg.dataSize()))
                {
                    if (_rmsg.type() < sizeof(ReservedTypes))
                        _rmsgs.push(_rmsg, _rmsg.priority());
                    else
                    {
                        ((Message::Fields*) _rmsg.raw())->header.type -= sizeof(ReservedTypes);
                        messages.push(_rmsg, _rmsg.priority());
                    }
                }
                _foundHead = false;
            }
        }
    }
    _rx->unlock();

    while (_rmsgs.size())
    {
        switch (_rmsgs.top().type())
        {
            case ReservedTypes::ping:
                _send(ReservedTypes::reping, _rmsgs.topPriority(), _rmsgs.top().data(), _rmsgs.top().dataSize());
                break;
            
            default:
                break;
        }
        _rmsgs.pop();
    }

    _tx->lock();
    while (_smsgs.size())
    {
        _tx->putUnlocked(_smsgs.top().raw(), _smsgs.top().size());
        _smsgs.pop();
    }
    _tx->unlock();
}

void USBMessageBroker::_send(uint8_t type, uint8_t priority, uint8_t* data, uint16_t len)
{
    Message msg(type, priority, data, len);
    _smsgs.push(msg, priority);
}
