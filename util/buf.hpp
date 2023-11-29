#include "buf.h"


template <unsigned LEN> void FIFOBuffer<LEN>::insert(uint8_t byte)
{
    uint8_t* data = (uint8_t*) this;
    for (unsigned i = 0; i < LEN - 1; i++)
        data[i] = data[i + 1];
    data[LEN - 1] = byte;
}

template <unsigned LEN> unsigned RingBuffer<LEN>::size()
{
    int size = _put - _get;
    if (size >= 0)
        return size;
    return LEN - size;
}

template <unsigned LEN> void RingBuffer<LEN>::put(uint8_t byte)
{
    _data[_put] = byte;
    _put = (_put + 1) % LEN;
    if (_put == _get)
        _get = (_get + 1) % LEN;
}

template <unsigned LEN> uint8_t RingBuffer<LEN>::get()
{
    uint8_t byte = _data[_get];
    if (_put != _get)
        _get = (_get + 1) % LEN;
    return byte;
}