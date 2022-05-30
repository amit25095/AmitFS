#pragma once

#include <cstdlib>
#include <cstdint>

class Disk
{
private:
    int fd;
    unsigned char* m_fileMap;
    uint32_t m_blockSize;
    uint32_t m_nblocks;

    void createDiskFile(const char* filePath);

public:
    Disk(const char* filePath, const uint32_t blockSize = 4096, const uint32_t nblocks = 4096);
    ~Disk();

    uint32_t getBlockSize() const { return m_blockSize; }
    uint32_t getBlocksAmount() const { return m_nblocks; }
    size_t getDiskSize() const { return m_blockSize * m_nblocks; }

    void read(unsigned long addr, int size, char* ans) const ;
    void write(unsigned long addr, int size, const char* data);
};