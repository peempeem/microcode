#include "sharedbuf.h"
#include <string.h>

SharedBuffer::SharedBuffer() : _buf(NULL), _size(0)
{

}

SharedBuffer::SharedBuffer(unsigned bytes) : _size(bytes)
{
    create();
}

SharedBuffer::SharedBuffer(unsigned bytes, uint8_t initializer) : _size(bytes)
{
    create();
    for (unsigned i = 0; i < bytes; i++)
        ((SharedBufferData*) _buf)->data[i] = initializer;
}

SharedBuffer::SharedBuffer(const uint8_t* data, unsigned bytes) : _size(bytes)
{
    create();
    memcpy(this->data(), data, bytes);
}

SharedBuffer::SharedBuffer(const SharedBuffer& other)
{
    copy(other);
}

SharedBuffer::~SharedBuffer()
{
    destroy();
}

void SharedBuffer::operator=(const SharedBuffer& other)
{
    destroy();
    copy(other);
}

unsigned SharedBuffer::users()
{
    ((SharedBufferData*) _buf)->users;
}

uint8_t* SharedBuffer::data()
{
    return ((SharedBufferData*) _buf)->data;
}

unsigned SharedBuffer::size()
{
    return _size;
}

void SharedBuffer::create()
{
    if (!_size)
    {
        _buf = NULL;
        return;
    }  

    _buf = new uint8_t[sizeof(SharedBufferData) + _size];
    ((SharedBufferData*) _buf)->lock = Lock();
    ((SharedBufferData*) _buf)->users = 1;
}

void SharedBuffer::destroy()
{
    if (!_buf)
        return;
    
    ((SharedBufferData*) _buf)->lock.lock();
    ((SharedBufferData*) _buf)->users -= 1;
    ((SharedBufferData*) _buf)->lock.unlock();
    if (!((SharedBufferData*) _buf)->users)
        delete[] _buf;
}

void SharedBuffer::copy(const SharedBuffer& other)
{
    _buf = other._buf;
    _size = other._size;
    if (_buf)
    {
        ((SharedBufferData*) _buf)->lock.lock();
        ((SharedBufferData*) _buf)->users += 1;
        ((SharedBufferData*) _buf)->lock.unlock();
    }
}
