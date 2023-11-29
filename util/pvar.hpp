#include "pvar.h"
#include "log.h"

template <class T> PVar<T>::PVar(const char* name) : _name(name)
{

}

template <class T> bool PVar<T>::load()
{
    if (!filesys.read(_name, (uint8_t*) &data, sizeof(T)))
    {
        log(_name, "Could not load from file");
        return false;
    }
    return true;
}

template <class T> bool PVar<T>::save()
{
    if (!filesys.write(_name, (uint8_t*) &data, sizeof(T)))
    {
        log(_name, "Could not write to file");
        return false;
    }
    return true;
}

