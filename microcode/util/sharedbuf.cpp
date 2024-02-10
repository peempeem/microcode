#include "sharedbuf.h"
#include <string.h>

SharedBuffer::SharedBuffer() : _buf(NULL)
{

}

SharedBuffer::SharedBuffer(unsigned bytes)
{
    _create(bytes);
}

SharedBuffer::SharedBuffer(unsigned bytes, uint8_t initializer)
{
    _create(bytes);
    for (unsigned i = 0; i < bytes; i++)
        ((SharedBufferData*) _buf)->data[i] = initializer;
}

SharedBuffer::SharedBuffer(const uint8_t* data, unsigned bytes)
{
    _create(bytes);
    memcpy(this->data(), data, bytes);
}

SharedBuffer::SharedBuffer(const SharedBuffer& other)
{
    _copy(other);
}

SharedBuffer::~SharedBuffer()
{
    _destroy();
}

void SharedBuffer::operator=(const SharedBuffer& other)
{
    _destroy();
    _copy(other);
}

unsigned SharedBuffer::users() const
{
    return _buf ? ((SharedBufferData*) _buf)->users : 0;
}

uint8_t* SharedBuffer::data() const
{
    return _buf ? ((SharedBufferData*) _buf)->data : NULL;
}

unsigned SharedBuffer::size() const
{
    return _buf ? ((SharedBufferData*) _buf)->size : 0;
}

bool SharedBuffer::allocated() const
{
    return _buf != NULL;
}

void SharedBuffer::_create(unsigned size)
{
    if (!size)
    {
        _buf = NULL;
        return;
    }  

    _buf = new uint8_t[sizeof(SharedBufferData) + size];
    ((SharedBufferData*) _buf)->lock = SpinLock();
    ((SharedBufferData*) _buf)->users = 1;
    ((SharedBufferData*) _buf)->size = size;
}

void SharedBuffer::_destroy()
{
    if (!_buf)
        return;
    
    ((SharedBufferData*) _buf)->lock.lock();
    ((SharedBufferData*) _buf)->users -= 1;
    ((SharedBufferData*) _buf)->lock.unlock();
    if (!((SharedBufferData*) _buf)->users)
        delete[] _buf;
}

void SharedBuffer::_copy(const SharedBuffer& other)
{
    _buf = other._buf;
    if (_buf)
    {
        ((SharedBufferData*) _buf)->lock.lock();
        ((SharedBufferData*) _buf)->users += 1;
        ((SharedBufferData*) _buf)->lock.unlock();
    }
}
