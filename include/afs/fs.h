#pragma once

#include <afs/disk.h>

class FileSystem
{
private:
    Disk* m_disk;

    void setHeader();

public:
    FileSystem(const char* filePath, size_t blockSize = 4096, size_t nblocks = 4096);
    ~FileSystem();

    void format();
};