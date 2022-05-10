#include <afs/fs.h>
#include <afs/bootLoad.h>
#include <afs/helper.h>
#include <afs/constants.h>

#include <iostream>

#include <cstring>
#include <cmath>

FileSystem::FileSystem(const char* filePath, uint32_t blockSize, uint32_t nblocks)
{
    m_header = BootLoad::load(filePath); // try to load header from existing file.
    
    if (!m_header)
    {
        blockSize = Helper::getCorrectSize(blockSize);
        m_header = new struct afsHeader;

        if (nblocks < MIN_BLOCKS_AMOUNT)
            nblocks = MIN_BLOCKS_AMOUNT;

        m_disk = new Disk(filePath, blockSize, nblocks);
        m_dblocksTable = new BlocksTable(m_disk, true);
        format();
    }

    else
    {
        m_disk = new Disk(filePath, m_header->blockSize, m_header->nblocks);
        m_dblocksTable = new BlocksTable(m_disk);
    }

}

FileSystem::~FileSystem()
{
    delete m_disk;
    delete m_header;
    delete m_dblocksTable;
}

/**
 * @brief Formats the default blocks the disk need to have (super block, blocks table, inodes table and root directory)
 * 
 */
void FileSystem::format()
{
    int defaultBlocks = 0;

    int dblocksTableAmount = m_disk->getBlocksAmount() / m_disk->getBlockSize(); // calculate the amounts of blocks needed for the blocks table. 

    setHeader(); // Set the superblock

    defaultBlocks = 1 + dblocksTableAmount + m_header->inodeBlocks; // Super Block + blocks table + inode table blocks

    for (int i = 0; i < defaultBlocks; i++)
        m_dblocksTable->reserveDBlock(i);
    
    // Create root directory
    createFile("/", true);
}

/**
 * @brief create a file or a directory in the file system.
 * 
 * @param path The path to create the file/directory in. 
 * @param isDir Boolean which tells us if the file is directory or not.
 */
void FileSystem::createFile(const std::string& path, const bool isDir)
{
    afsPath parsedPath = Helper::splitString(path);
    std::string fileName = parsedPath[parsedPath.size() - 1];


    // create inode for the file.
    inode fileInode = {
        .flags = isDir ? DIRTYPE : FILETYPE,
        .fileSize = 0,
        .firstAddr = (address)-1
    };

    if (!isDir) 
        createInode(fileInode);

    // Create current directory(.) and parent directory(..)
    // in case the file is a directory.
    else
    {
        unsigned int fileBlock = m_dblocksTable->getFreeBlock();
        unsigned int currentInode = m_header->inodes, prevInode;


        m_dblocksTable->reserveDBlock(fileBlock);
        fileInode.firstAddr = Helper::blockToAddr(m_disk->getBlockSize(), fileBlock);

        createInode(fileInode);

        if (path == "/")
            prevInode = currentInode;

        else
        {
            address prevParentAddr = pathToAddr(afsPath(parsedPath.begin(), parsedPath.end() - 2));
            dirSibling parent = getSiblingData(prevParentAddr, *(path.end() - 2));
            prevInode = parent.indodeTableIndex;
        }

        createCurrAndPrevDir(currentInode, prevInode);
    }

    // Add the new file as a sibling to the parent directory.
    if (path != "/")
    {
        address parentAddr = pathToAddr(afsPath(parsedPath.begin(), parsedPath.end() - 1));

        dirSibling file;

        strncpy(file.name, fileName.c_str(), sizeof(file.name));
        file.indodeTableIndex = m_header->inodes - 1;

        addSibling(parentAddr, file);
    }
}

/**
 * @brief append content to a file.
 * 
 * @param filePath the path to the file.
 * @param content the content to write to the file.
 * 
 */
void FileSystem::appendContent(const std::string& filePath, std::string content)
{
    inode fileInode = pathToInode(Helper::splitString(filePath));
    address fileAddr = 0, currentAddr;
    uint32_t blockSize = m_disk->getBlockSize(), remainingForCurrentBlock = blockSize - (fileInode.fileSize % blockSize) - 4;

    if (fileInode.flags & DIRTYPE) 
        throw std::runtime_error("cant write content to a directory");

    // in case of empty file, reserve a data block for it's content
    if (fileInode.firstAddr == (address)-1)
    {
        unsigned int dataBlock = m_dblocksTable->getFreeBlock();
        m_dblocksTable->reserveDBlock(dataBlock);
        fileInode.firstAddr = Helper::blockToAddr(blockSize, dataBlock);
    }

    fileAddr = fileInode.firstAddr;
    currentAddr = Helper::getLastFileBlock(m_disk, fileAddr);

    // Fragmentize the file into multiple blocks if needed
    while (content.size() > remainingForCurrentBlock)
    {
        std::string currentPart = content.substr(0, remainingForCurrentBlock);
        content = content.substr(remainingForCurrentBlock, content.size());
        unsigned int nextBlock = m_dblocksTable->getFreeBlock();
        m_dblocksTable->reserveDBlock(nextBlock);

        currentAddr = Helper::blockToAddr(blockSize, nextBlock);

        // write the needed content and the next address to the current block.
        m_disk->write(fileAddr + blockSize - remainingForCurrentBlock - sizeof(address), remainingForCurrentBlock, currentPart.c_str());
        m_disk->write(fileAddr + blockSize - sizeof(address), sizeof(address), (char*)&currentAddr);

        // update the file size in the inode
        fileInode.fileSize += currentPart.size();
        remainingForCurrentBlock = m_disk->getBlockSize() - sizeof(address);

        // move to the next node (address) in the linked list (file)
        fileAddr = currentAddr;
    }

    m_disk->write(fileAddr, content.size(), content.c_str());
    fileInode.fileSize += content.size();

    afsPath path = Helper::splitString(filePath);
    int fileInodeIdx = getSiblingData(pathToAddr(afsPath(path.begin(), path.end() - 1)), path[path.size() - 1]).indodeTableIndex;

    m_disk->write(inodeIndexToAddr(fileInodeIdx), sizeof(inode), (const char*)&fileInode);
}

/**
 * @brief free blocks of a specified file and add deleted flag to it.
 * 
 * @param filePath the path of the file to delete
 * 
 */
void FileSystem::deleteFile(const std::string& filePath)
{
    char reset[sizeof(dirSibling)] = { 0 };
    afsPath path = Helper::splitString(filePath);
    int fileInodeIdx = getSiblingData(pathToAddr(afsPath(path.begin(), path.end() - 1)), path[path.size() - 1]).indodeTableIndex;
    inode fileInode = pathToInode(path);

    address currentAddr = fileInode.firstAddr, parentAddress = pathToAddr(afsPath(path.begin(), path.end() - 1));
    directoryData data;
    dirSibling sibling, lastSibling;

    m_disk->read(parentAddress, sizeof(directoryData), (char*)&data);

    fileInode.flags |= DELETED;

    while (currentAddr != 0)
    {
        m_dblocksTable->freeDBlock(Helper::addrToBlock(m_disk->getBlockSize(), currentAddr));
        m_disk->read(currentAddr + m_disk->getBlockSize() - 4, sizeof(address), (char*)&currentAddr);
        m_disk->write(currentAddr + m_disk->getBlockSize() - 4, sizeof(address), (const char*)reset);
    }

    m_disk->write(inodeIndexToAddr(fileInodeIdx), sizeof(inode), (const char*)&fileInode);


    address lastSiblingAddr = Helper::getSiblingAddr(parentAddress, data - 1);

    lastSibling = getSiblingData(parentAddress, data - 1);
    m_disk->write(lastSiblingAddr, sizeof(dirSibling), reset);

    for (directoryData i = 0; i < data; i++)
    {
        sibling = getSiblingData(parentAddress, i);
        if (strncmp(sibling.name, path.back().c_str(), sizeof(sibling.name)) == 0)
        {
            m_disk->write(Helper::getSiblingAddr(parentAddress, i), sizeof(dirSibling), (const char*)&lastSibling);
            m_disk->write(lastSiblingAddr, sizeof(dirSibling), reset);
            break;
        }
    }

    m_disk->write(parentAddress, sizeof(directoryData), (const char*)&(--data));
}

/**
 * @brief Get content of a file
 *
 * @param filePath path to the file to get the content of
 *
 * @return std::string The content of the requested file.
 */
std::string FileSystem::getContent(const std::string &filePath) const
{
    inode fileInode = pathToInode(Helper::splitString(filePath));
    address currentAddress = fileInode.firstAddr;
    std::string fileContent = "", temp = "";
    uint32_t size = fileInode.fileSize, blockSize = m_disk->getBlockSize();
    char* buffer = new char[blockSize - sizeof(address)];

    while (currentAddress != 0)
    {
        if (size > blockSize)
        {
            m_disk->read(currentAddress, blockSize - sizeof(address), buffer);
            temp.assign(buffer, blockSize - sizeof(address));
            size -= (blockSize - sizeof(address));
        }

        else
        {
            delete[] buffer;
            buffer = new char[size];
            m_disk->read(currentAddress, size, buffer);
            temp.assign(buffer, size);
        }

        fileContent += temp;
        m_disk->read(currentAddress + blockSize - sizeof(address), sizeof(address), (char*)&currentAddress);
    }

    return fileContent;
}

dirList FileSystem::listDir(const std::string &dirPath) const
{
    dirList list;
    inode dirInode = pathToInode(Helper::splitString(dirPath)), siblingInode;
    directoryData data;

    if (dirInode.flags & DELETED)
        throw std::runtime_error("cant list deleted directory");

    if (dirInode.flags & FILETYPE)
        throw std::runtime_error("cannot list a file that is not a directory!");

    m_disk->read(dirInode.firstAddr, sizeof(directoryData), (char*)&data);

    for (directoryData i = 0; i < data; i++)
    {
        dirSibling sibling = getSiblingData(dirInode.firstAddr, i);
        m_disk->read(inodeIndexToAddr(sibling.indodeTableIndex), sizeof(inode), (char*)&siblingInode);

        dirListEntry entry;
        strncpy(entry.name, sibling.name, sizeof(sibling.name));
        entry.isDirectory = siblingInode.flags & DIRTYPE;
        entry.fileSize = siblingInode.fileSize;

        list.push_back(entry);
    }

    return list;
}

// =========== Helpers (private functions) =========== //

/**
* @brief write the superblock to the newly created disk
*
*/
void FileSystem::setHeader()
{
    memcpy(m_header->magic, MAGIC, sizeof(m_header->magic));
    m_header->version = CURR_VERSION;
    m_header->blockSize = m_disk->getBlockSize();
    m_header->nblocks = m_disk->getBlocksAmount();
    m_header->inodeBlocks = ceil(m_disk->getBlocksAmount() / 10);
    m_header->inodes = 0;

    m_disk->write(0, sizeof(struct afsHeader), (const char*)m_header);
}


/**
* @brief Write newly created inode into the disk.
*
* @param node The inode to write to the disk.
*
*/
void FileSystem::createInode(const inode node)
{
    m_disk->write(inodeIndexToAddr(m_header->inodes), sizeof(node), (const char*)&node);
    m_header->inodes++;
    m_disk->write(0, sizeof(struct afsHeader), (const char*)m_header);
}

/**
* @brief Convert inode index to address in the inode table.
*
* @param inodeIndex Index of the inode to get the address of.
*
* @return int The address of the inode in the inode table.

*/
int FileSystem::inodeIndexToAddr(const int inodeIndex) const
{
    return ((1 + m_dblocksTable->getTableBlocksAmount()) * m_disk->getBlockSize()) + (sizeof(inode) * inodeIndex);
}


/**
 * @brief get the root directory 
 * 
 * @return FileSystem::inode the inode of the root directory. 
 */
inode FileSystem::getRoot() const
{
    inode root;

    m_disk->read(inodeIndexToAddr(0), sizeof(inode), (char*)&root);

    return root;
}

/**
 * @brief Get the sibling of a directory by the index of the sibling in the directory.
 * 
 * @param dirAddr the parent directory address of the sibling.
 * @param indx the index of the sibling in the directory.
 * 
 * @return FileSystem::dirSibling the sibling with all the needed data.
 */
dirSibling FileSystem::getSiblingData(const address dirAddr, const int indx) const
{
    dirSibling sibling;
    uint16_t maxSiblingsPerBlock = (m_disk->getBlockSize() - sizeof(directoryData) - sizeof(address)) / sizeof(dirSibling);
    unsigned int blockNum = indx / maxSiblingsPerBlock;
    address currentAddr = dirAddr;
    int offset = sizeof(dirSibling) * (indx - maxSiblingsPerBlock * blockNum);

    if (blockNum == 0)
        offset += sizeof(directoryData);

    for (unsigned int i = 0; i < blockNum; i++)
        m_disk->read(currentAddr + m_disk->getBlockSize() - sizeof(address), sizeof(address), (char*)&currentAddr);
    
    m_disk->read(currentAddr + offset, sizeof(dirSibling), (char*)&sibling);
    
    return sibling;
}

/**
 * @brief Get the sibling of a directory by the name of the sibling in the directory.
 * 
 * @param dirAddr the parent directory address of the sibling.
 * @param siblingName the name of the sibling in the directory.
 * 
 * @return FileSystem::dirSibling the sibling with all the needed data.
 */
dirSibling FileSystem::getSiblingData(const address dirAddr, const std::string& siblingName) const
{
    dirSibling sibling;
    directoryData data;
    bool found = false;

    m_disk->read(dirAddr, sizeof(directoryData), (char*)&data);

    for (uint16_t i = 2; i < data && !found; i++)
    {
        sibling = getSiblingData(dirAddr, i);
        if (strncmp(sibling.name, siblingName.c_str(), sizeof(sibling.name)) == 0)
            found = true;
    }

    if (!found)
    {
        throw std::runtime_error(std::string("could not find file: ") + siblingName);
    }

    return sibling;
}

/**
 * @brief convert path in the filesystem into an address.
 * 
 * @param path path to the file we want to get the address of. 
 * 
 * @return address the address of the requested path.
 */
address FileSystem::pathToAddr(const afsPath path) const
{
    inode curr;
    directoryData data;
    directorySibling sibling;

    int currentSubDir = 1;
    uint16_t i; 
    int j;

    curr = getRoot();

    m_disk->read(curr.firstAddr, sizeof(directoryData), (char*)&data);

    for(i = 1; i < path.size(); i++)
    {
        for (j = 0; j < data; j++)
        {
            sibling = getSiblingData(curr.firstAddr, j);
            if (std::string(sibling.name) == path[currentSubDir])
            {
                m_disk->read(inodeIndexToAddr(sibling.indodeTableIndex), sizeof(inode), (char*)&curr);
                break;
            }
        }
        if (j == data)
            throw std::runtime_error(std::string("No such file exist with name: ") + path[i]);
        
        if(i != path.size() - 1 && !(curr.flags & DIRTYPE))
            throw std::runtime_error("path contains file that is not a directory.");
    } 


    return curr.firstAddr;
}

/**
 * @brief get free chunk address inside a directory. 
 * 
 * @param dirAddr the address of the dir. 
 * 
 * @return address the available chunk address.
 */
address FileSystem::getFreeDirChunkAddr(const address dirAddr)
{
    directoryData data;
    address lastAddr = Helper::getLastFileBlock(m_disk, dirAddr), chunkAddress = 0;
    uint16_t maxSiblingsPerBlock = (m_disk->getBlockSize() - sizeof(directoryData) - sizeof(address)) / sizeof(dirSibling);
    unsigned int dirBlocks;
    

    m_disk->read(dirAddr, sizeof(directoryData), (char*)&data);
    dirBlocks = floor(data / maxSiblingsPerBlock) + 1;
    
    unsigned int siblingsRemainInBlock =  (maxSiblingsPerBlock * dirBlocks) - (maxSiblingsPerBlock * (dirBlocks - 1)) - data;

    chunkAddress = lastAddr + sizeof(dirSibling) * (maxSiblingsPerBlock - siblingsRemainInBlock);
    if (lastAddr == dirAddr)
        chunkAddress += sizeof(directoryData);

    return chunkAddress;
}

/**
 * @brief add a sibling to a directory. 
 * 
 * @param dirAddr the address of the directory to add the sibling to. 
 * @param sibling the sibling to add to the directory.
 * 
 */
void FileSystem::addSibling(const address dirAddr, const dirSibling sibling)
{
    directoryData data;
    address currentAddress = dirAddr;
    uint16_t maxSiblingsPerBlock = (m_disk->getBlockSize() - sizeof(directoryData) - sizeof(address)) / sizeof(dirSibling);

    unsigned int dirBlocks;

    m_disk->read(dirAddr, sizeof(directoryData), (char*)&data);
    currentAddress = Helper::getLastFileBlock(m_disk, currentAddress);

    dirBlocks = floor(data / maxSiblingsPerBlock) + 1;
    if (data + 1 > (uint16_t)(maxSiblingsPerBlock * dirBlocks))
    {
        unsigned int nextBlock = m_dblocksTable->getFreeBlock();
        address nextBlockAddr = Helper::blockToAddr(m_disk->getBlockSize(), currentAddress);
        m_dblocksTable->reserveDBlock(nextBlock);
        m_disk->write(currentAddress + m_disk->getBlockSize() - sizeof(address), sizeof(address), (const char*)&nextBlockAddr);
    }

    m_disk->write(getFreeDirChunkAddr(dirAddr), sizeof(dirSibling), (const char*)&sibling);
    m_disk->write(dirAddr, sizeof(directoryData), (const char*)&(++data));
}


/**
 * @brief create previous (..) and current (.) dir and add it to the needed directory 
 * 
 * @param currentDirInode the inode number of the current directory
 * @param prevDirInode the inode number of the previous directory
 * 
 */
void FileSystem::createCurrAndPrevDir(const unsigned int currentDirInode, const unsigned int prevDirInode)
{
    dirSibling curr, prev;
    inode currentDir, prevDir;
    directoryData data;

    // read the inodes of the current and previous directories
    m_disk->read(inodeIndexToAddr(currentDirInode), sizeof(inode), (char*)&currentDir);
    m_disk->read(inodeIndexToAddr(prevDirInode), sizeof(inode), (char*)&prevDir);
    
    // read the data of the current directory
    m_disk->read(currentDir.firstAddr, sizeof(directoryData), (char*)&data);

    // add the current and previous directories as siblings to the current dir.
    curr.indodeTableIndex = currentDirInode;
    strncpy(curr.name, ".", sizeof(curr.name));

    prev.indodeTableIndex = prevDirInode;
    strncpy(prev.name, "..", sizeof(prev.name));

    addSibling(currentDir.firstAddr, curr);
    addSibling(currentDir.firstAddr, prev);
}

/**
 * @brief convert file path to inode. 
 * 
 * @param path the path of the file to get the inode of.
 * 
 * @return FileSystem::inode the inode of the requested file.
 */
inode FileSystem::pathToInode(const afsPath path) const 
{
    if (path[path.size() - 1] == "/")
        return getRoot();

    afsPath parentPath = afsPath(path.begin(), path.end() - 1);
    address parentAddr = pathToAddr(parentPath);

    dirSibling sibling = getSiblingData(parentAddr, path[path.size() - 1]);
    
    inode siblingInode;

    m_disk->read(inodeIndexToAddr(sibling.indodeTableIndex), sizeof(inode), (char*)&siblingInode);

    return siblingInode;
}
