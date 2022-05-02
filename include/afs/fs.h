#pragma once

#include <afs/disk.h>
#include <afs/constants.h>
#include <cstdint>

#define DBLOCKS_TABLE_BLOCK_INDX 1

class FileSystem
{
private:
    Disk* m_disk;
    struct afsHeader* m_header;
    int m_dblocksTableAmount;
    bool* m_dblocksTable;

    enum InodeFlags
    {
        DELETED = 0,
        FILETYPE,
        DIRTYPE
    };

    typedef struct inode
    {
        int flags;
        uint32_t fileSize;
        uint32_t firstAddr;
    } inode;

    typedef struct directorySibling
    {
        char name[NAME_MAX_LEN]; 
        uint32_t indodeTableIndex;
    } dirSibling;
    
    unsigned long blockToAddr(unsigned int blockNum, unsigned int offset = 0) const;
    unsigned int getFreeBlock() const;
    int inodeIndexToAddr(int inodeIndex) const;
    void setHeader();
    void createInode(inode node);
    void reserveDBlock(unsigned int blockNum);
    void createRootDir();

public:
    FileSystem(const char* filePath, uint32_t blockSize = 4096, uint32_t nblocks = 4096);
    ~FileSystem();

    void format();
};