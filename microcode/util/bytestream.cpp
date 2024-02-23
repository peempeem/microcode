#include "bytestream.h"

void ByteStream::put(const uint8_t* buf, unsigned len)
{
    lock();
    if (_bufs.empty())
        _bufs.emplace();
    ByteStreamBuffer* sbuf = &_bufs.back();
    unlock();

    unsigned written = 0;
    while (written != len)
    {
        if (sbuf->read == (unsigned) sizeof(ByteStreamBuffer::data))
        {
            lock();
            _bufs.emplace();
            sbuf = &_bufs.back();
            unlock();
        }

        sbuf->data[sbuf->read] = buf[written++];
        sbuf->read++;
    }
}

void ByteStream::put(std::streambuf& buf)
{
    lock();
    if (_bufs.empty())
        _bufs.emplace();
    ByteStreamBuffer* sbuf = &_bufs.back();
    unlock();

    int next = buf.sbumpc();
    while (next != EOF)
    {
        if (sbuf->read == (unsigned) sizeof(ByteStreamBuffer::data))
        {
            lock();
            _bufs.emplace();
            sbuf = &_bufs.back();
            unlock();
        }

        sbuf->data[sbuf->read] = next;
        sbuf->read++;
        next = buf.sbumpc();
    }
}

bool ByteStream::isEmpty()
{
    lock();
    bool ret = _bufs.empty() || _bufs.size() > 1 || _bufs.front().read == _bufs.front().write;
    unlock();
    return ret;
}

unsigned ByteStream::get(uint8_t* buf, unsigned max)
{
    lock();
    if (_bufs.empty())
    {
        unlock();
        return 0;
    }
    
    ByteStreamBuffer* sbuf = &_bufs.front();
    unlock();

    unsigned read = 0;
    while (read != max)
    {
        if (sbuf->write == sbuf->read)
        {
            if (sbuf->write == (unsigned) sizeof(ByteStreamBuffer::data))
            {
                lock();
                _bufs.pop();
                
                if (_bufs.empty())
                {
                    unlock();
                    break;
                }

                sbuf = &_bufs.front();
                unlock();

                if (sbuf->write >= sbuf->read)
                    break;
            }
            else
                break;
        }

        buf[read++] = sbuf->data[sbuf->write++];
    }
    return read;
}
