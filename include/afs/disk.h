#pragma once

#include <cstdlib>

class Disk
{
private:
    int fd;
    unsigned char* m_fileMap;
    size_t m_blockSize;
    size_t m_nblocks;

    void createDiskFile(const char* filePath);

public:
    Disk(const char* filePath, const size_t blockSize = 4096, const size_t nblocks = 4096);
    ~Disk();

    size_t getBlockSize() { return m_blockSize; }
    size_t getBlocksAmount() { return m_nblocks; }
    size_t getDiskSize() { return m_blockSize * m_nblocks; }

    void read(unsigned long addr, int size, char* ans);
    void write(unsigned long addr, int size, const char* data);
};