#pragma once

#include <afs/disk.h>
#include <afs/constants.h>

#include <vector>
#include <string>

#include <cstdint>

#define DBLOCKS_TABLE_BLOCK_INDX 1

typedef std::vector<std::string> afsPath;
typedef uint16_t directoryData;

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
    uint32_t blockToAddr(unsigned int blockNum, unsigned int offset = 0) const;
    
    int inodeIndexToAddr(int inodeIndex) const;
    uint32_t pathToAddr(afsPath path) const;
    uint32_t getFreeDirChunkAddr(uint32_t dirAddr);

    unsigned int getFreeBlock() const;
    void setHeader();
    void createInode(inode node);
    void reserveDBlock(unsigned int blockNum);
    void createRootDir();
    afsPath parsePath(std::string path_str) const;
    inode getRoot() const;
    dirSibling getSiblingData(const uint32_t dirAddr, int indx) const;


public:
    FileSystem(const char* filePath, uint32_t blockSize = 4096, uint32_t nblocks = 4096);
    ~FileSystem();

    void format();
    void createFile(const std::string& path, bool isDir = false);
};