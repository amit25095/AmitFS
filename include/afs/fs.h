#pragma once

#include <afs/disk.h>
#include <afs/blocksTable.h>
#include <afs/constants.h>
#include <afs/fsStructs.h>

#include <vector>
#include <string>

#include <cstdint>

class FileSystem
{
private:
    Disk* m_disk;
    struct afsHeader* m_header;
    BlocksTable* m_dblocksTable;
    
    address inodeIndexToAddr(const int inodeIndex) const;
    address pathToAddr(const afsPath path) const;
    address getFreeDirChunkAddr(const address dirAddr);
    inode getRoot() const;
    inode pathToInode(const afsPath path) const;
    dirSibling getSiblingData(const address dirAddr, const int indx) const;
    dirSibling getSiblingData(const address dirAddr, const std::string& siblingName) const;

    void setHeader();

    void createCurrAndPrevDir(const unsigned int currentDirInode, const unsigned int prevDirInode);
    void createInode(const inode node);
    void addSibling(const address dirAddr, const dirSibling sibling);

public:
    FileSystem(const char* filePath, uint32_t blockSize = 4096, uint32_t nblocks = 4096);
    ~FileSystem();

    void format();
    void createFile(const std::string& path, const bool isDir = false);
    void appendContent(const std::string& filePath, std::string content);
    void deleteFile(const std::string& filePath);
    std::string getContent(const std::string& filePath) const;
    dirList listDir(const std::string& dirPath) const;
};