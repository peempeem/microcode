#include "filesys.h"
#include "log.h"
#include <SPIFFS.h>

const static char* LogHeader = "filesys";

bool FileSys::init(bool format) {
    return SPIFFS.begin(true, "/spiffs", 32);
}

bool FileSys::read(const char* path, uint8_t* buf, int size, int seek)
{
    File file = SPIFFS.open(path);
    if (!file) 
    {
        file.close();
        Log(LogHeader) << path << " does not exist";
        return false;
    }
    
    file.seek(seek, SeekMode::SeekCur);
    unsigned nbytes = file.read(buf, size);
    file.close();
    if (nbytes != size)
    {
        Log(LogHeader) << path << " read less bytes [" << nbytes << "] than requested [" << size << "]";
        return false;
    }
    return true;
}

bool FileSys::write(const char* path, uint8_t* buf, int size, int seek)
{
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file)
        Log(LogHeader) << "File does not exist, creating " << path;
    
    file.seek(seek, SeekMode::SeekCur);
    int nbytes = file.write(buf, size);
    file.close();
    if (nbytes != size)
    {
        Log(LogHeader) << "Wrote less bytes [" << nbytes << "] than requested [" << size << "]";
        return false;
    }
    return true;
}

bool FileSys::remove(const char* path)
{
    if (SPIFFS.remove(path))
    {
        Log(LogHeader) << "Removed " << path;
        return true;
    }
    Log(LogHeader) << "Failed to remove " << path;
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
        files.emplace_back(file.path(), file.size());
        file.close();
        file = root.openNextFile();
    }
    root.close();
    return files;
}

bool FileSys::clear()
{
    Log log(LogHeader);
    log << "Clearing filesystem";
    std::vector<FileData> files = map("/");
    for (FileData& file : files)
    {
        if (strlen(file.name()) == 1)
            continue;
        if (!remove(file.name()))
        {
            log.failed();
            return false;
        }       
    }
    log.success();
    return true;
}

void FileSys::logMap(std::vector<FileSys::FileData> files)
{
    Log log(LogHeader);
    log << "Enumerating files...\n";
    log << "   Size    |    Path   \n";
    unsigned len = 0;
    for (FileData& fd : files)
    {
        unsigned pathLen = strlen(fd.name());
        if (pathLen > len)
            len = pathLen;
    }
    log << std::setw(max(23, (int) len + 13) + 1) << std::setfill('=') << "\n" << std::setfill(' ');
    for (FileData& fd : files)
        log << std::setw(10) << fd.size() << std::setw(0) << " | " << fd.name() << "\n";
    log.flush(false);
}
