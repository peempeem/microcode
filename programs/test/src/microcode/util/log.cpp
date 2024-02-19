#include "log.h"
#include "../hal/hal.h"

static ByteStream* reserves;
static ByteStream* stream = NULL;
static BinarySemaphore lock = BinarySemaphore();

static void setStream(ByteStream* newStream)
{
    lock.lock();
    if (reserves)
        newStream->put(*reserves);
    stream = newStream;
    lock.unlock();
}

static void writeDebug(std::streambuf* buf)
{
    lock.lock();
    if (stream)
        stream->put(*buf);
    else
    {
        if (!reserves)
            reserves = new ByteStream();
        reserves->put(*buf);
    }
    lock.unlock();
}

Log::Log() : _flushed(true) 
{

}

Log::Log(const char* header, bool nlOnFlush, unsigned headerPadding) : _flushed(false), _nl(nlOnFlush)
{
    uint64_t time = sysTime();
    unsigned s = time / 1000000;
    unsigned ms = (time / 1000) % 1000;
    _ss << '[' << std::setw(5) << std::setfill(' ') << s << '.';
    _ss << std::setw(3) << std::setfill('0') << ms << "] [";
    _ss << std::setw(2) << uxTaskPriorityGet(NULL) << '|' << xPortGetCoreID() << "] [";
    _ss << std::setw(headerPadding) << std::setfill(' ') << pcTaskGetName(NULL) << "] [";
    _ss << std::setw(headerPadding) << std::setfill(' ') << header << "]: ";
    _headlen = _ss.tellp();
}

Log::Log(const Log& other)
{
    _ss << other._ss.rdbuf();
}

Log::~Log()
{
    flush();
}

Log& Log::operator<<(std::string str)
{
    for (unsigned i = 0; i < str.size(); ++i)
    {
        if (str[i] == '\n')
            _ss << "\n" << std::setw(_headlen) << std::setfill(' ') << ' ';
        else
            _ss << str[i];
    }
    return *this;
}

Log& Log::operator<<(const char* str)
{
    while (*str)
    {
        if (*str == '\n')
            _ss << "\n" << std::setw(_headlen) << std::setfill(' ') << ' ';
        else
            _ss << *str;
        str++;
    }
    return *this;
}

Log& Log::operator>>(ByteStream& newStream)
{
    setStream(&newStream);
    return *this;
}

void Log::flush()
{
    if (_flushed)
        return;
    
    if (_nl)
        _ss << "\n";
    
    writeDebug(_ss.rdbuf());
    _ss.clear();
    _flushed = true;
}

void Log::success()
{
    _ss << " -> [ SUCCESS ]";
    flush();
}

void Log::failed()
{
    _ss << " -> [ FAILED ]";
    flush();
}
