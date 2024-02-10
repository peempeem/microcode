#include "log.h"
#include "../hal/hal.h"

static ByteStream* reserves;
static ByteStream* stream = NULL;
static BinarySemaphore lock = BinarySemaphore();

void setStream(ByteStream* newStream)
{
    lock.lock();
    if (reserves)
        newStream->put(*reserves);
    stream = newStream;
    lock.unlock();
}

void writeDebug(std::streambuf* buf)
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

Log::Log(const char* header, unsigned headerPadding) : std::stringstream(), _flushed(false)
{
    uint64_t time = sysTime();
    unsigned s = time / 1000000;
    unsigned ms = (time / 1000) % 1000;
    (*this) << '[' << std::setw(5) << std::setfill(' ') << s << '.';
    (*this) << std::setw(3) << std::setfill('0') << ms << "] [";
    (*this) << std::setw(2) << uxTaskPriorityGet(NULL) << '|' << xPortGetCoreID() << "] [";
    (*this) << std::setw(headerPadding) << std::setfill(' ') << header << "]: ";
}

Log& Log::operator=(const Log& other)
{
    flush();
    (*this) << other.rdbuf();
    return *this;
}

Log& Log::operator>>(ByteStream& newStream)
{
    setStream(&newStream);
    return *this;
}

void Log::flush(bool nl)
{
    if (_flushed)
        return;
    
    if (nl)
        (*this) << "\n";
    writeDebug(rdbuf());
    clear();
    _flushed = true;
}

void Log::success()
{
    (*this) << " -> [ SUCCESS ]";
    flush();
}

void Log::failed()
{
    (*this) << " -> [ FAILED ]";
    flush();
}
