#include "filesys.h"
#include "log.h"
#include <SPIFFS.h>

const static char* LogHeader = "filesys";

bool FileSys::init(bool format)
{
    _lock.lock();
    bool ret = SPIFFS.begin(true, "/spiffs", 32);
    _lock.unlock();
    _init = ret;
    return ret;
}

bool FileSys::isInitialized()
{
    return _init;
}

bool FileSys::read(const char* path, uint8_t* buf, int size, int seek)
{
    _lock.lock();
    File file = SPIFFS.open(path);
    if (!file) 
    {
        file.close();
        _lock.unlock();
        Log(LogHeader) << path << " does not exist";
        return false;
    }
    
    file.seek(seek, SeekMode::SeekCur);
    unsigned nbytes = file.read(buf, size);
    file.close();
    _lock.unlock();
    if (nbytes != size)
    {
        Log(LogHeader) << path << " read less bytes [" << nbytes << "] than requested [" << size << "]";
        return false;
    }
    return true;
}

bool FileSys::write(const char* path, uint8_t* buf, int size, int seek)
{
    _lock.lock();
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file)
        Log(LogHeader) << "File does not exist, creating " << path;
    
    file.seek(seek, SeekMode::SeekCur);
    int nbytes = file.write(buf, size);
    file.close();
    _lock.unlock();
    if (nbytes != size)
    {
        Log(LogHeader) << "Wrote less bytes [" << nbytes << "] than requested [" << size << "]";
        return false;
    }
    return true;
}

bool FileSys::remove(const char* path)
{
    _lock.lock();
    bool ret = SPIFFS.remove(path);
    _lock.unlock();
    if (ret)
        Log(LogHeader) << "Removed " << path;
    else
        Log(LogHeader) << "Failed to remove " << path;
    return ret;
}

bool FileSys::clear()
{
    Log log(LogHeader);
    log << "Clearing filesystem";
    std::vector<FileData> files = map("/");
    _lock.lock();
    for (FileData& file : files)
    {
        if (strlen(file.name()) == 1)
            continue;
        if (!remove(file.name()))
        {
            _lock.unlock();
            log.failed();
            return false;
        }       
    }
    _lock.unlock();
    log.success();
    return true;
}

std::vector<FileSys::FileData> FileSys::map(const char* path)
{
    std::vector<FileData> files;
    _lock.lock();
    File root = SPIFFS.open(path);
    if (!root.isDirectory())
    {
        _lock.unlock();
        return files;
    }

    File file = SPIFFS.open(path);
    while (file)
    {
        files.emplace_back(file.path(), file.size());
        file.close();
        file = root.openNextFile();
    }
    root.close();
    _lock.unlock();
    return files;
}

std::string FileSys::toString()
{
    std::vector<FileData> files = map("/");
    std::stringstream ss;
    ss << "Enumerating files...\n";
    ss << "   Size    |    Path   \n";

    unsigned len = 0;
    for (FileData& fd : files)
    {
        unsigned pathLen = strlen(fd.name());
        if (pathLen > len)
            len = pathLen;
    }

    ss << std::setw(max(23, (int) len + 13) + 1) << std::setfill('=') << "=" << "\n" << std::setfill(' ');
    for (FileData& fd : files)
        ss << std::setw(10) << fd.size() << " | " << fd.name() << "\n";
    return ss.str();
}
