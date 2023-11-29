#pragma once

#include "filesys.h"

template <class T> class PVar
{
    public:
        T data;

        PVar(const char* name);

        bool load();
        bool save();
    
    private:
        const char* _name;
};

#include "pvar.hpp"
