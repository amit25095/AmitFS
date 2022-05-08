#pragma once

#include <afs/disk.h>
#include <afs/constants.h>

#include <vector>
#include <string>

#include <cstdint>

#define DBLOCKS_TABLE_BLOCK_INDX 1

typedef std::vector<std::string> afsPath;
typedef uint16_t directoryData;
typedef uint32_t address;

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
        address firstAddr;
    } inode;

    typedef struct directorySibling
    {
        char name[NAME_MAX_LEN]; 
        uint32_t indodeTableIndex;
    } dirSibling;
    
    uint32_t blockToAddr(unsigned int blockNum, unsigned int offset = 0) const;
    
    int inodeIndexToAddr(int inodeIndex) const;
    address pathToAddr(afsPath path) const;
    address getFreeDirChunkAddr(address dirAddr);
    unsigned int getFreeBlock() const;
    inode getRoot() const;
    inode pathToInode(afsPath path) const;
    dirSibling getSiblingData(const address dirAddr, int indx) const;
    dirSibling getSiblingData(const address dirAddr, const std::string& siblingName) const;

    void setHeader();

    void createCurrAndPrevDir(unsigned int currentDirInode, unsigned int prevDirInode);
    void createInode(inode node);
    void createRootDir();
    void addSibling(address dirAddr, dirSibling sibling);

    void reserveDBlock(unsigned int blockNum);
    afsPath parsePath(std::string path_str) const;


public:
    FileSystem(const char* filePath, uint32_t blockSize = 4096, uint32_t nblocks = 4096);
    ~FileSystem();

    void format();
    void createFile(const std::string& path, bool isDir = false);
    void appendContent(const std::string& filePath, std::string content);
};