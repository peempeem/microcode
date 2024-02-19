#pragma once

#include "lock.h"
#include "bytestream.h"
#include <ostream>
#include <sstream>
#include <iomanip>
#include <streambuf>

class Log
{
    public:
        Log();
        Log(const char* header, bool nlOnFlush=true, unsigned headerPadding=16);
        Log(const Log& other);
        ~Log();

        Log& operator<<(std::string str);
        Log& operator<<(const char* str);
        template <class T> Log& operator<<(T val)
        {
            _ss << val;
            return *this;
        }
        Log& operator>>(ByteStream& stream);

        void flush();

        void success();
        void failed();

    private:
        std::stringstream _ss;
        unsigned _headlen;
        bool _flushed;
        bool _nl;
};
