#include "crypt.h"

#define FNVOB   0x811c9dc5
#define FNVP    0x01000193

unsigned hash32(const uint8_t* data, unsigned len)
{
    uint8_t value[4];
    *((uint32_t*) value) = FNVOB;
    
    for (unsigned i = 0; i < len; ++i)
    {
        *((uint32_t*) value) *= FNVP;
        value[0] ^= data[i];
    }
    return *((uint32_t*) value);
}
