#pragma once

#include <cstdlib>

class Disk
{
private:
    char* m_fileMap;
    size_t m_diskSize;

    inline static const size_t BLOCK_SIZE = 4096;

public:
    Disk(const char* filePath, const size_t blockSize = 4096, const size_t nblocks = 4096);
    ~Disk();
};