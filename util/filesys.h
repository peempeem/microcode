#pragma once

#include <stdint.h>
#include <vector>

class FileSys
{
    public:
        class FileData
        {
            public:
                FileData(char* name, unsigned size);
                FileData(const char* name, unsigned size);
                FileData(const FileData& other);
                ~FileData();
                
                const char* name();
                unsigned size();

            private:
                char* _name;
                unsigned _size;

                void init(const char* name);
        };

        bool init(bool format=true);
        bool read(const char* path, uint8_t* buf, int size, int seek=0);
        bool write(const char* path, uint8_t* buf, int size, int seek=0);
        bool remove(const char* path);
        std::vector<FileData> map(const char* path);
        bool clear();
        void logMap(std::vector<FileData> files);
} static filesys;
