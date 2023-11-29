#include "usbcom.h"
#include "../util/crypt.h"
#include <string.h>

/*
USBMessage::USBMessage(unsigned type, const uint8_t* data, unsigned len)
{
    _buf = SharedBuffer(sizeof(msg_t) + len);
    ((msg_t*) _buf.data())->header.data.type = type;
    ((msg_t*) _buf.data())->header.data.len = len;
    ((msg_t*) _buf.data())->header.data.hash = hash32(data, len);
    ((msg_t*) _buf.data())->header.hash = hash32((uint8_t*) &((msg_t*) _buf.data())->header.data, sizeof(USBMSG::HEADER::DATA));
    memcpy(((msg_t*) _buf.data())->data, data, len);
}

USBMessage::USBMessage(SharedBuffer& buf)
{
    _buf = buf;
}

unsigned USBMessage::type()
{
    return ((msg_t*) _buf.data())->header.data.type;
}

uint8_t* USBMessage::data()
{
    return ((msg_t*) _buf.data())->data;
}

unsigned USBMessage::dataSize()
{
    return ((msg_t*) _buf.data())->header.data.len;
}

uint8_t* USBMessage::buffer()
{
    return _buf.data();
}

unsigned USBMessage::bufferSize()
{
    return _buf.size();
}

USBMessageBroker::USBMessageBroker(unsigned port, float ramSendRate) : _port(port), _ramSendRate(ramSendRate)
{
    
}

void USBMessageBroker::init(unsigned baudrate)
{
    sysSerialInit(_port, baudrate);
}

USBMessage& USBMessageBroker::front()
{
    return _inq.front();
}

void USBMessageBroker::pop()
{
    _inqLock.lock();
    _inq.pop();
    _inqLock.unlock();
}

bool USBMessageBroker::size()
{
    return _inq.size();
}

void USBMessageBroker::send(unsigned type, const uint8_t* data, unsigned len)
{
    _outqLock.lock();
    _outq.emplace(type, data, len);
    _outqLock.unlock();
}

void USBMessageBroker::update()
{
    handleInbound();
    handleOutbound();
}

void USBMessageBroker::handleInbound()
{

}

void USBMessageBroker::handleOutbound()
{
    while (!_outq.empty())
    {
        sysSerialWrite(_port, _outq.front().buffer(), _outq.front().bufferSize());
        _outqLock.lock();
        _outq.pop();
        _outqLock.unlock();
    }
}
*/