#pragma once

#include "lock.h"
#include "ringbuf.h"
#include <queue>
#include <streambuf>

#define ByteStreamInternalSplit 256

class ByteStream : public BinarySemaphore
{
    public:
        ByteStream() : BinarySemaphore() {}

        bool isEmptyUnlocked() { return !_bufs.size() || !_bufs.front().size(); }

        unsigned sizeUnlocked()
        {
            switch (_bufs.size())
            {
                case 0:
                    return 0;
                
                case 1:
                    return _bufs.front().size();
                
                default:
                    return _bufs.front().size() + (_bufs.size() - 2) * ByteStreamInternalSplit + _bufs.back().size();
            }
        }

        void putUnlocked(uint8_t byte)
        {
            if (_bufs.empty() || _bufs.back().isFull())
                _bufs.emplace();
                    
            _bufs.back().put(byte);
        }

        uint8_t getUnlocked()
        {
            uint8_t byte = _bufs.front().get();
            if (!_bufs.front().size())
                _bufs.pop();
            return byte;
        }

        bool isEmpty()
        {
            lock();
            bool empty = isEmptyUnlocked();
            unlock();
            return empty;
        }

        unsigned size()
        {
            lock();
            unsigned sz = sizeUnlocked();
            unlock();
            return sz;
        }

        void put(uint8_t byte)
        {
            lock();
            putUnlocked(byte);
            unlock();
        }

        void put(uint8_t* bytes, unsigned len)
        {
            lock();
            for (unsigned i = 0; i < len; i++)
                putUnlocked(bytes[i]);
            unlock();
        }

        void put(std::streambuf& buf)
        {
            lock();
            int next = buf.sbumpc();
            while (next != EOF)
            {
                putUnlocked(next);
                next = buf.sbumpc();
            }
            unlock();
        }

        void put(ByteStream& bytestream)
        {
            lock();
            bytestream.lock();
            while (!bytestream.isEmptyUnlocked())
                putUnlocked(bytestream.getUnlocked());
            bytestream.unlock();
            unlock();
        }

        uint8_t get()
        {
            lock();
            uint8_t byte = getUnlocked();
            unlock();
            return byte;
        }

        unsigned get(uint8_t* bytes, unsigned maxLen)
        {
            lock();
            unsigned i;
            for (i = 0; i < maxLen; i++)
            {
                if (isEmptyUnlocked())
                    break;
                bytes[i] = getUnlocked();
            }
            unlock();
            return i;
        }

    private:
        std::queue<StaticRingBuffer<ByteStreamInternalSplit>> _bufs;
};
