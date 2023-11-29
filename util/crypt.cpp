#include "crypt.h"

#define FNVOB   0x811c9dc5
#define FNVP    0x01000193

unsigned hash32(const uint8_t* data, unsigned len)
{
    union
    {
        struct   
        {
            uint8_t zero;
            uint8_t one;
            uint8_t two;
            uint8_t three;
        } bytes;
        unsigned value = FNVOB;   
    } hash;
    
    for (unsigned i = 0; i < len; i++)
    {
        hash.value *= FNVP;
        hash.bytes.zero ^= data[i];
    }
    return hash.value;
}
