#pragma once

#include <stdint.h>

class RingBuffer
{
    public:
        RingBuffer(unsigned len) : _maxSize(len) { _data = new uint8_t[++_maxSize]; }
        ~RingBuffer() { delete _data; }

        bool empty() { return !_size; }
        unsigned size() { return _size; }
        bool isFull() { return _size == _maxSize - 1; }

        void put(uint8_t byte)
        {
            _data[_put] = byte;
            _put = (_put + 1) % _maxSize;
            if (_put == _get)
                _get = (_get + 1) % _maxSize;
            else
                _size++;
        }

        uint8_t get()
        {
            uint8_t byte = _data[_get];
            if (_put != _get)
            {
                _get = (_get + 1) % _maxSize;
                _size--;
            }
            return byte;
        }
    
    private:
        unsigned _maxSize;
        unsigned _size = 0;
        unsigned _put = 0;
        unsigned _get = 0;
        uint8_t* _data;
};

template <unsigned LEN> class StaticRingBuffer
{
    public:
        bool empty() { return _put == _get; }
        unsigned size() { return _size; }
        bool isFull() { return _size == LEN; }

        void put(uint8_t byte)
        {
            _data[_put] = byte;
            _put = (_put + 1) % (LEN + 1);
            if (_put == _get)
                _get = (_get + 1) % (LEN + 1);
            else
                _size++;
        }

        uint8_t get()
        {
            uint8_t byte = _data[_get];
            if (_put != _get)
            {
                _get = (_get + 1) % (LEN + 1);
                _size--;
            }
            return byte;
        }
    
    private:
        unsigned _size = 0;
        unsigned _put = 0;
        unsigned _get = 0;
        uint8_t _data[LEN + 1];
};