#include "svector.h"

template <class T> unsigned SVector<T>::deserialize(uint8_t* data)
{
    std::vector<T>::clear();
    unsigned len = *((uint16_t*) data);
    std::vector<T>::reserve(len);
    T* ptr = (T*) (data + sizeof(uint16_t));
    for (unsigned i = 0; i < len; i++)
        std::vector<T>::push_back(*(ptr + i));
    return serialSize();
}

template <class T> unsigned SVector<T>::serialize(uint8_t* data)
{
    *((uint16_t*) data) = std::vector<T>::size();
    T* ptr = (T*) (data + sizeof(uint16_t));
    for (unsigned i = 0; i < std::vector<T>::size(); i++)
        *(ptr + i) = (*this)[i];
    return serialSize();
}