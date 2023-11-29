#include "filesys.h"
#include "log.h"
#include <string.h>

#if __has_include(<SPIFFS.h>)

const static char* logHeader = "filesys";

FileSys::FileData::FileData(char* name, unsigned size) : _size(size)
{
    init((const char*) name);
}

FileSys::FileData::FileData(const char* name, unsigned size) : _size(size)
{
    init(name);
}

FileSys::FileData::FileData(const FileSys::FileData& other) : _size(other._size)
{
    init((const char*) other._name);
}

FileSys::FileData::~FileData()
{
    delete[] _name;
}

const char* FileSys::FileData::name()
{
    return _name;
}

void FileSys::FileData::init(const char* name)
{
    _name = new char[strlen(name) + 1];
    strcpy(_name, name);
}

unsigned FileSys::FileData::size()
{
    return _size;
}

bool FileSys::init(bool format) {
    log(logHeader, "Loading filesystem", false);

    if (!SPIFFS.begin(true, "/spiffs", 32))
    {
        logf();
        log(logHeader, "Failed to mount SPIFFS filesystem (first boot / bad flash)");
        return false;
    }
    logs();
    return true;
}

bool FileSys::read(const char* path, uint8_t* buf, int size, int seek)
{

    File file = SPIFFS.open(path);
    if (!file) 
    {
        file.close();
        log(logHeader, path, false);
        logc(" does not exist");
        return false;
    }
    
    file.seek(seek, SeekMode::SeekCur);
    unsigned nbytes = file.read(buf, size);
    file.close();
    if (nbytes != size)
    {
        log(logHeader, path, false);
        logc(" read less bytes [", false);
        logc(nbytes, false);
        logc("] than requested [", false);
        logc(size, false);
        logc("]");
        return false;
    }
    return true;
}

bool FileSys::write(const char* path, uint8_t* buf, int size, int seek)
{
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file)
    {
        log(logHeader, "File does not exist, creating ", false);
        logc(path);
    }
    
    file.seek(seek, SeekMode::SeekCur);
    int nbytes = file.write(buf, size);
    file.close();
    if (nbytes != size)
    {
        log(logHeader, "Wrote less bytes [", false);
        logc(nbytes, false);
        logc("] than requested [", false);
        logc(size, false);
        logc("]");
        return false;
    }
    return true;
}

bool FileSys::remove(const char* path)
{
    if (SPIFFS.remove(path))
    {
        log(logHeader, "Removed ", false);
        logc(path);
        return true;
    }
    log(logHeader, "Failed to remove ", false);
    logc(path);
    return false;
}

std::vector<FileSys::FileData> FileSys::map(const char* path)
{
    std::vector<FileData> files;
    File root = SPIFFS.open(path);
    if (!root.isDirectory())
        return files;
    
    File file = SPIFFS.open(path);
    while (file)
    {
        files.emplace_back(file.name(), file.size());
        file.close();
        file = root.openNextFile();
    }
    root.close();
    return files;
}

bool FileSys::clear() {
    log(logHeader, "Clearing filesystem...");
    std::vector<FileData> files = map("/");
    for (FileData& file : files)
    {
        if (strlen(file.name()) == 1)
            continue;
        if (!remove(file.name()))
        {
            log(logHeader, "Couldn't clear filesystem");
            return false;
        }       
    }
    log(logHeader, "Cleared filesystem");
    return true;
}

void FileSys::logMap(std::vector<FileSys::FileData> files)
{
    log(logHeader, "Enumerating files...");
    char buf[12];
    for (FileData& fd : files)
    {
        logc("Size: ", false);
        int len = snprintf(buf, 12, "%d", fd.size());
        logc(buf, false);
        int j;
        for (j = 0; j < 11 - len; j++)
            buf[j] = ' ';
        buf[j] = 0;
        logc(buf, false);
        logc(fd.name());
    }
    log(logHeader, "Done enumerating");
}

#endif
