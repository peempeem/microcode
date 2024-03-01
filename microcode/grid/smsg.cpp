#include "smsg.h"

//
//// SharedGridBuffer Class
//

SharedGridBuffer::SharedGridBuffer()
{

}

SharedGridBuffer::SharedGridBuffer(std::string name, unsigned priority, unsigned expire) : Mutex(), _name(name), _priority(priority), _expire(expire)
{
    
}

const std::string& SharedGridBuffer::name()
{
    return _name;
}

unsigned SharedGridBuffer::priority()
{
    return _priority;
}

SharedBuffer& SharedGridBuffer::touch(unsigned id)
{
    Extras& ex = data[id];
    ex.arrival = sysTime();
    ex.send.time = ex.arrival;
    ex.readFrom = true;
    ex.writtenTo = true;
    return ex.buf;
}

bool SharedGridBuffer::canWrite(unsigned id, bool clear)
{
    Extras* ex = data.at(id);
    if (ex && ex->writtenTo)
    {
        if (clear)
            ex->writtenTo = false;
        return true;
    }
    return false;
}

bool SharedGridBuffer::canRead(unsigned id, bool clear)
{
    Extras* ex = data.at(id);
    if (ex && !ex->readFrom)
    {
        if (clear)
            ex->readFrom = true;
        return true;
    }
    return false;
}

unsigned SharedGridBuffer::serializeName(uint8_t* ptr)
{
    uint8_t* end = ptr;
    *((uint8_t*) end) = _name.size();
    end += sizeof(uint8_t);

    if (_name.empty())
        return end - ptr;

    memcpy(end, &_name[0], _name.size());
    end += _name.size();
    return end - ptr;
}

unsigned SharedGridBuffer::serializeIDS(uint8_t* ptr, unsigned id)
{
    uint8_t* end = ptr;
    Extras* ex = data.at(id);
    if (!ex)
    {
        *((uint16_t*) end) = 0;
        end += sizeof(uint16_t);
        return end - ptr;
    }

    *((uint16_t*) end) = 1;
    end += sizeof(uint16_t);

    *((uint16_t*) end) = id;
    end += sizeof(uint16_t);

    *((Extras::Send*) end) = ex->send;
    end += sizeof(Extras::Send);
    
    *((uint16_t*) end) = ex->buf.size();
    end += sizeof(uint16_t);

    memcpy(end, ex->buf.data(), ex->buf.size());
    end += ex->buf.size();
    return end - ptr;
}

unsigned SharedGridBuffer::serializeIDS(uint8_t* ptr)
{
    uint8_t* end = ptr;
    uint16_t* numIDS = ((uint16_t*) end);
    end += sizeof(uint16_t);

    unsigned ids = 0;
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        if (!canWrite(it.key()))
            continue;
        ids++;

        *((uint16_t*) end) = it.key();
        end += sizeof(uint16_t);

        *((Extras::Send*) end) = it->send;
        end += sizeof(Extras::Send);
        
        *((uint16_t*) end) = it->buf.size();
        end += sizeof(uint16_t);

        memcpy(end, it->buf.data(), it->buf.size());
        end += it->buf.size();
    }

    *numIDS = ids;
    return end - ptr;
}

unsigned SharedGridBuffer::serialSize(unsigned id)
{
    unsigned size = sizeof(uint8_t) + _name.size() + sizeof(uint16_t);
    
    Extras* ex = data.at(id);
    if (!ex)
        return size;

    size += sizeof(uint16_t) + sizeof(Extras::Send) + sizeof(uint16_t) + ex->buf.size();
    return size;
}

unsigned SharedGridBuffer::serialSize()
{
    unsigned size = sizeof(uint8_t) + _name.size() + sizeof(uint16_t);
    
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        if (canWrite(it.key(), false))
            size += sizeof(uint16_t) + sizeof(Extras::Send) + sizeof(uint16_t) + it->buf.size();
    }
    return size;
}

unsigned SharedGridBuffer::deserializeName(uint8_t* ptr, std::string& str)
{
    uint8_t* end = ptr;
    unsigned nameLength = *((uint8_t*) end);
    end++;

    if (!nameLength)
        str.clear();

    str = std::string((char*) end, nameLength);
    end += nameLength;
    return end - ptr;
}

unsigned SharedGridBuffer::deserializeIDS(uint8_t* ptr)
{
    uint8_t* end = ptr;
    unsigned ids = *((uint16_t*) end);
    end += sizeof(uint16_t);

    for (unsigned i = 0; i < ids; ++i)
    {
        unsigned id = *((uint16_t*) end);
        Extras& ex = data[*((uint16_t*) end)];
        end += sizeof(uint16_t);

        Extras::Send send = *((Extras::Send*) end);
        end += sizeof(Extras::Send);

        if (send.time > ex.send.time)
        {
            ex.arrival = sysTime();
            ex.readFrom = false;
            ex.writtenTo = false;
            ex.send = send;
            ex.buf = SharedBuffer(*((uint16_t*) end));
            end += sizeof(uint16_t);
            memcpy(ex.buf.data(), end, ex.buf.size());
            end += ex.buf.size();
        }
        else
            end += sizeof(uint16_t) + *((uint16_t*) end);
    }
    return end - ptr;
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
