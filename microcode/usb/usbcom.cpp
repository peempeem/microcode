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

//
//// USBMessageBroker::Message Class
//

USBMessageBroker::Message::Message()
{

}

USBMessageBroker::Message::Message(
    uint8_t type, 
    uint8_t priority, 
    const uint8_t* data, 
    uint16_t len)
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

bool USBMessageBroker::Message::operator<(const Message& other) const
{
    return get().header.priority < get().header.priority;
}

USBMessageBroker::Message::Fields& USBMessageBroker::Message::get() const
{
    return *((Fields*) _buf.data());
}

uint8_t* USBMessageBroker::Message::raw()
{
    return _buf.data();
}

unsigned USBMessageBroker::Message::size()
{
    return _buf.size();
}

//
//// USBMessageBroker::MessageQueue Class
//

void USBMessageBroker::MessageQueue::push(Message& msg)
{
    MinPriorityQueue<Message>::push(msg, msg.get().header.priority);
}

//
//// USBMessageBroker Class
//

void USBMessageBroker::attachStreams(ByteStream* tx, ByteStream* rx)
{
    _tx = tx;
    _rx = rx;
}

void USBMessageBroker::send(uint8_t type, uint8_t priority, uint8_t* data, uint16_t len)
{
    _send(type + sizeof(ReservedTypes), priority + 1, data, len);
}

void USBMessageBroker::update()
{
    if (!_tx || !_rx)
        return;
    
    uint8_t buf[256];
    unsigned pulled;
    do
    {
        pulled = _rx->get(buf, sizeof(buf));
        for (unsigned i = 0; i < pulled; ++i)
        {
            if (!_foundHead)
            {
                _buf.insert(buf[i]);
                if (((Message::Fields::Header*) _buf.data)->hhash == hash32(_buf.data, sizeof(Message::Fields::Header) - sizeof(Message::Fields::Header::hhash)))
                {
                    _rmsg = Message(*((Message::Fields::Header*) _buf.data));
                    _idx = 0;
                    if (!_rmsg.get().header.len)
                    {
                        if (_rmsg.get().header.type < sizeof(ReservedTypes))
                            _rmsgs.push(_rmsg, _rmsg.get().header.priority);
                        else
                        {
                            _rmsg.get().header.type -= sizeof(ReservedTypes);
                            messages.lock();
                            messages.push(_rmsg);
                            messages.unlock();
                        }
                    }
                    else
                        _foundHead = true;                
                }
            }
            else
            {
                _rmsg.get().data[_idx++] = buf[i];
                if (_idx >= _rmsg.get().header.len)
                {
                    if (((Message::Fields*) _rmsg.raw())->header.dhash == hash32(_rmsg.get().data, _rmsg.get().header.len))
                    {
                        if (_rmsg.get().header.type < sizeof(ReservedTypes))
                            _rmsgs.push(_rmsg, _rmsg.get().header.priority);
                        else
                        {
                            _rmsg.get().header.type -= sizeof(ReservedTypes);
                            messages.lock();
                            messages.push(_rmsg);
                            messages.unlock();                       
                        }
                    }
                    _foundHead = false;
                }
            }
        }
        
    } while (pulled);

    while (!_rmsgs.empty())
    {
        Message& msg = _rmsgs.top();
        switch (msg.get().header.type)
        {
            case ReservedTypes::ping:
                _send(
                    ReservedTypes::reping, 
                    msg.get().header.priority, 
                    msg.get().data, 
                    msg.get().header.len);
                break;
            
            default:
                break;
        }
        _rmsgs.pop();
    }

    _smsgs.lock();
    while (!_smsgs.empty())
    {
        _tx->put(_smsgs.top().raw(), _smsgs.top().size());
        _smsgs.pop();
    }
    _smsgs.unlock();
}

void USBMessageBroker::_send(uint8_t type, uint8_t priority, uint8_t* data, uint16_t len)
{
    Message msg(type, priority, data, len);
    _smsgs.lock();
    _smsgs.push(msg);
    _smsgs.unlock();
}
