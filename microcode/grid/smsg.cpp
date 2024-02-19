#include "smsg.h"

SharedGridBuffer::SharedGridBuffer(std::string name, unsigned expire) : BinarySemaphore(), _name(name), _expire(expire)
{
    
}

const std::string& SharedGridBuffer::name()
{
    return _name;
}

SharedBuffer& SharedGridBuffer::touch(unsigned id)
{
    Extras& ex = data[id];
    ex.arrival = sysTime();
    ex.send.time = ex.arrival;
    ex.isNew = true;
    return ex.buf;
}

bool SharedGridBuffer::isNew(unsigned id)
{
    Extras* ex = data.at(id);
    if (ex && ex->isNew)
    {
        ex->isNew = false;
        return true;
    }
    return false;
}

void SharedGridBuffer::serializeName(uint8_t*& ptr)
{
    *((uint8_t*) ptr) = _name.size();
    ptr += sizeof(uint8_t);

    if (_name.empty())
        return;

    memcpy(ptr, &_name[0], _name.size());
    ptr += _name.size();
}

void SharedGridBuffer::serializeID(uint8_t*& ptr, unsigned id)
{
    Extras* ex = data.at(id);
    if (!ex)
    {
        *((uint16_t*) ptr) = 0;
        ptr += sizeof(uint16_t);
    }

    *((uint16_t*) ptr) = id;
    ptr += sizeof(uint16_t);

    *((Extras::Send*) ptr) = ex->send;
    ptr += sizeof(Extras::Send);
    
    *((uint16_t*) ptr) = ex->buf.size();
    ptr += sizeof(uint16_t);

    memcpy(ptr, ex->buf.data(), ex->buf.size());
    ptr += ex->buf.size();
}

unsigned SharedGridBuffer::serialSize(unsigned id)
{
    unsigned size = sizeof(uint8_t);
    if (_name.empty())
        return size;
    
    size += sizeof(uint8_t) + _name.size() + sizeof(uint16_t);
    
    Extras* ex = data.at(id);
    if (!ex)
        return size;

    size += sizeof(Extras::Send) + sizeof(uint16_t) +  ex->buf.size();
    return size;
}

std::string SharedGridBuffer::deserializeName(uint8_t*& ptr)
{
    unsigned nameLength = *((uint8_t*) ptr);
    ptr += sizeof(uint8_t);

    if (!nameLength)
        std::string();

    std::string s((char*) ptr, nameLength);
    ptr += nameLength;
    return s;
}

#include "../util/log.h"

void SharedGridBuffer::deserializeID(uint8_t*& ptr)
{
    unsigned id = *((uint16_t*) ptr);
    ptr += sizeof(uint16_t);

    Log("id") << id;

    if (!id)
        return;
    
    Extras& ex = data[id];
    Extras::Send send = *((Extras::Send*) ptr);
    ptr += sizeof(Extras::Send);

    if (send.time > ex.send.time)
    {
        ex.arrival = sysTime();
        ex.send = send;
        ex.buf = SharedBuffer(*((uint16_t*) ptr));
        ptr += sizeof(uint16_t);
        memcpy(ex.buf.data(), ptr, ex.buf.size());
        ptr += ex.buf.size();
    }
    else
        ptr += sizeof(uint16_t) + *((uint16_t*) ptr);
}

void SharedGridBuffer::preen()
{
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        if (sysTime() > it->arrival + _expire)
            data.remove(it);
    }
    data.shrink();
}
