#pragma once

#include <afs/disk.h>
#include <afs/constants.h>
#include <cstdint>

class FileSystem
{
private:
    Disk* m_disk;
    size_t m_inodeBlocks;

    enum class FileType
    {
        DELETED = 0,
        FILETYPE,
        DIRTYPE
    };

    typedef struct inode
    {
        FileType type;
        uint32_t fileSize;
        uint32_t firstAddr;
    } inode;
    
    unsigned long blockToAddr(unsigned int blockNum, unsigned int offset = 0);
    void setHeader();

public:
    FileSystem(const char* filePath, uint32_t blockSize = 4096, uint32_t nblocks = 4096);
    ~FileSystem();

    void format();
};