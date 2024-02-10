#pragma once

#include <stdint.h>
#include <string>

class MAC
{
    public:
        uint8_t addr[6];

        MAC();
        MAC(const uint8_t* addr);
        MAC(const MAC& other);


        void operator=(const MAC& other);
        bool operator==(const MAC& other);

        void set(const uint8_t* mac);
        void set(const MAC& mac);

        bool is(const uint8_t* addr);
        bool is(const MAC& mac);
        
        std::string toString();
};
