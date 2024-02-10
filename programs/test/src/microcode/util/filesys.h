#pragma once

#include <stdint.h>
#include <string.h>
#include <vector>

class FileSys
{
    public:
        class FileData
        {
            public:
                FileData(char* name, unsigned size) : _size(size) { _init((const char*) name); }
                FileData(const char* name, unsigned size) : _size(size) { _init(name); }
                FileData(const FileData& other) : _size(other._size) { _init((const char*) other._name); }
                ~FileData() { delete[] _name; };
                
                const char* name() { return _name; }
                unsigned size() { return _size; }

            private:
                char* _name;
                unsigned _size;

                void _init(const char* name)
                {
                    _name = new char[strlen(name) + 1];
                    strcpy(_name, name);
                }
        };

        bool init(bool format=true);
        bool read(const char* path, uint8_t* buf, int size, int seek=0);
        bool write(const char* path, uint8_t* buf, int size, int seek=0);
        bool remove(const char* path);
        std::vector<FileData> map(const char* path);
        bool clear();
        void logMap(std::vector<FileData> files);
} static filesys;
