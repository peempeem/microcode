#include "fifo.h"

template <unsigned LEN> void InPlaceFIFOBuffer<LEN>::insert(uint8_t byte)
{
    for (unsigned i = 0; i < LEN - 1; i++)
        ((uint8_t*) this)[i] = ((uint8_t*) this)[i + 1];
    ((uint8_t*) this)[LEN - 1] = byte;
}