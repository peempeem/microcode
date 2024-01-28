#pragma once

#include "lock.h"
#include "bytestream.h"
#include <ostream>
#include <sstream>
#include <iomanip>
#include <streambuf>

class Log : public std::stringstream
{
    public:
        Log();
        Log(const char* header, unsigned headerPadding=16);
        Log(const Log& other) { (*this) << other.rdbuf(); }
        ~Log() { flush(); }

        Log& operator=(const Log& other);

        Log& operator>>(ByteStream& stream);

        void flush(bool nl=true);

        void failed();
        void success();

    private:
        bool _flushed;
};
