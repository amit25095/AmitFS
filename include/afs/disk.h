#pragma once

#include <cstdlib>

class Disk
{
private:
    int fd;
    unsigned char* m_fileMap;
    size_t m_diskSize;

    inline static const size_t BLOCK_SIZE = 4096;

    void createDiskFile(const char* filePath);

public:
    Disk(const char* filePath, const size_t blockSize = 4096, const size_t nblocks = 4096);
    ~Disk();

    void read(unsigned long addr, int size, char* ans);
    void write(unsigned long addr, int size, const char* data);
};