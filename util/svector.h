#pragma once

#include <stdint.h>
#include <vector>

template <class T> class SVector : public std::vector<T>
{
    public:
        SVector() : std::vector<T>() {}
        
        unsigned deserialize(uint8_t* data);
        unsigned serialSize() { return sizeof(unsigned) + sizeof(T) * std::vector<T>::size(); }
        unsigned serialize(uint8_t* data);
};

#include "svector.hpp"
