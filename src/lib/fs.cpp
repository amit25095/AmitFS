#include <afs/fs.h>
#include <afs/bootLoad.h>
#include <afs/helper.h>
#include <afs/constants.h>

#include <iostream>
#include <sstream>

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
        format();
    }

    else
    {
        m_disk = new Disk(filePath, m_header->blockSize, m_header->nblocks);
        m_dblocksTableAmount = m_disk->getBlocksAmount() / m_disk->getBlockSize();
        m_dblocksTable = new bool[m_dblocksTableAmount * m_disk->getBlockSize()];
        for (int i = 0; i < m_dblocksTableAmount; i++)
            m_disk->read(blockToAddr(DBLOCKS_TABLE_BLOCK_INDX), m_disk->getBlockSize(), (char*)(m_dblocksTable + (i * m_disk->getBlockSize())));
    }


}

FileSystem::~FileSystem()
{
    delete m_disk;
    delete m_header;
    delete[] m_dblocksTable;
}

/**
 * @brief Formats the default blocks the disk need to have (super block, blocks table, inodes table and root directory)
 * 
 */
void FileSystem::format()
{
    uint32_t blockSize = m_disk->getBlockSize();
    int defaultBlocks = 0;

    m_dblocksTableAmount = m_disk->getBlocksAmount() / m_disk->getBlockSize(); // calculate the amounts of blocks needed for the blocks table. 

    setHeader(); // Set the superblock

    defaultBlocks = 1 + m_dblocksTableAmount + m_header->inodeBlocks; // Super Block + blocks table + inode table blocks

    // Set the blocks table and writes it to the disk
    bool dblocksTable[blockSize * m_dblocksTableAmount] = { false };
    for (int i = 0; i < defaultBlocks; i++)
        dblocksTable[i] = true;

    m_disk->write(blockToAddr(1), blockSize, (const char*)dblocksTable);
    m_dblocksTable = new bool[blockSize * m_dblocksTableAmount];
    memcpy(m_dblocksTable, dblocksTable, blockSize * m_dblocksTableAmount);
    
    // Create root directory
    createFile("/", true);
}

/**
 * @brief create a file or a directory in the file system.
 * 
 * @param path The path to create the file/directory in. 
 * @param isDir Boolean which tells us if the file is directory or not.
 */
void FileSystem::createFile(const std::string& path, bool isDir)
{
    afsPath parsedPath = parsePath(path);
    std::string fileName = parsedPath[parsedPath.size() - 1];


    // create inode for the file.
    inode fileInode = {
        .flags = isDir ? (1 << DIRTYPE) : FILETYPE,
        .fileSize = 0,
        .firstAddr = (address)-1
    };

    if (!isDir) 
        createInode(fileInode);

    // Create current directory(.) and parent directory(..)
    // in case the file is a directory.
    else
    {
        unsigned int fileBlock = getFreeBlock();
        unsigned int currentInode = m_header->inodes, prevInode;

        reserveDBlock(fileBlock);
        fileInode.firstAddr = blockToAddr(fileBlock);

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
        address subdirAddr = pathToAddr(afsPath(parsedPath.begin(), parsedPath.end() - 1));

        dirSibling file;

        strncpy(file.name, fileName.c_str(), sizeof(file.name));
        file.indodeTableIndex = m_header->inodes - 1;

        addSibling(subdirAddr, file);
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
    inode fileInode = pathToInode(parsePath(filePath));
    address fileAddr = 0, currentAddr = fileAddr;
    uint32_t blockSize = m_disk->getBlockSize(), remainingForCurrentBlock = blockSize - (fileInode.fileSize % blockSize) - 4;

    if (fileInode.flags & (1 << DIRTYPE))
        throw std::runtime_error("cant write content to a directory");

    // in case of empty file, reserve a data block for it's content
    if (fileInode.firstAddr == (address)-1)
    {
        unsigned int dataBlock = getFreeBlock();
        reserveDBlock(dataBlock);
        fileInode.firstAddr = blockToAddr(dataBlock);
    }

    fileAddr = fileInode.firstAddr;

    // get to the currently last part of the file.
    while (currentAddr != 0)
    {
        fileAddr = currentAddr;
        m_disk->read(currentAddr + m_disk->getBlockSize() - 4, sizeof(address), (char*)&currentAddr);
    }

    // Fragmentize the file into multiple blocks if needed
    while (content.size() > remainingForCurrentBlock)
    {
        std::string currentPart = content.substr(0, remainingForCurrentBlock);
        content = content.substr(remainingForCurrentBlock, content.size());
        unsigned int nextBlock = getFreeBlock();
        reserveDBlock(nextBlock);

        currentAddr = blockToAddr(nextBlock);

        // write the needed content and the next address to the current block.
        m_disk->write(fileAddr, remainingForCurrentBlock, currentPart.c_str());
        m_disk->write(fileAddr + remainingForCurrentBlock, sizeof(address), (char*)&currentAddr);

        // update the file size in the inode
        fileInode.fileSize += currentPart.size();
        remainingForCurrentBlock = m_disk->getBlockSize() - 4;

        // move to the next node (address) in the linked list (file)
        fileAddr = currentAddr;
    }

    m_disk->write(currentAddr, content.size(), content.c_str());
    fileInode.fileSize += content.size();

    afsPath path = parsePath(filePath);
    int fileInodeIdx = getSiblingData(pathToAddr(afsPath(path.begin(), path.end() - 1)), path[path.size() - 1]).indodeTableIndex;

    m_disk->write(inodeIndexToAddr(fileInodeIdx), sizeof(inode), (const char*)&fileInode);
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
* @brief Find available data block to use
*
* @return unsigned int The number of the found block.
*/
unsigned int FileSystem::getFreeBlock() const
{
    unsigned int i = 0;
    bool found = false;

    for (i = 0; i < m_dblocksTableAmount * m_disk->getBlockSize() && !found; i++)
    {
        if (!m_dblocksTable[i])
            found = true;
    }

    return found ? i - 1 : -1;
}

/**
* @brief Write newly created inode into the disk.
*
* @param node The inode to write to the disk.
*
*/
void FileSystem::createInode(inode node)
{
    m_disk->write(inodeIndexToAddr(m_header->inodes), sizeof(node), (const char*)&node);
    m_header->inodes++;
    m_disk->write(0, sizeof(struct afsHeader), (const char*)m_header);
}

/**
* @brief reserve data block in the blocks table.
*
* @param blockNum The number of block to reserve.
*/
void FileSystem::reserveDBlock(unsigned int blockNum)
{
    m_dblocksTable[blockNum] = true;
    m_disk->write(blockToAddr(1, blockNum), sizeof(bool), (const char*)(m_dblocksTable + blockNum));
}


/**
* @brief Convert block number into address on the disk.
*
* @param blockNum Number of block to get the address of.
* @param offset Offset in the block.

* @return address The address of the block + the offset.

*/
address FileSystem::blockToAddr(unsigned int blockNum, unsigned int offset) const
{
    return (blockNum * m_disk->getBlockSize()) + offset;
}

/**
* @brief Convert inode index to address in the inode table.
*
* @param inodeIndex Index of the inode to get the address of.
*
* @return int The address of the inode in the inode table.

*/
int FileSystem::inodeIndexToAddr(int inodeIndex) const
{
    return ((1 + m_dblocksTableAmount) * m_disk->getBlockSize()) + (sizeof(inode) * inodeIndex);
}

/**
 * @brief parse the path into parts (seperated by /) 
 * 
 * @param pathStr the path to parse 
 * 
 * @return afsPath vector of strings with the path splitted 
 */
afsPath FileSystem::parsePath(std::string pathStr) const
{
	std::stringstream ss(pathStr);
	std::string part;
	afsPath ans;

	while (std::getline(ss, part, '/'))
		ans.push_back(part);

	return ans;
}

/**
 * @brief get the root directory 
 * 
 * @return FileSystem::inode the inode of the root directory. 
 */
FileSystem::inode FileSystem::getRoot() const
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
FileSystem::dirSibling FileSystem::getSiblingData(const address dirAddr, int indx) const
{
    dirSibling sibling;
    
    m_disk->read((dirAddr + sizeof(directoryData)) + (indx * sizeof(dirSibling)), sizeof(dirSibling), (char*)&sibling);
    
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
FileSystem::dirSibling FileSystem::getSiblingData(const address dirAddr, const std::string& siblingName) const
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
        sibling.indodeTableIndex = -1;
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
address FileSystem::pathToAddr(afsPath path) const
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
        
        if(i != path.size() - 1 && !(curr.flags & (1 << DIRTYPE)))
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
address FileSystem::getFreeDirChunkAddr(address dirAddr)
{
    directoryData data;
    m_disk->read(dirAddr, sizeof(directoryData), (char*)&data);
    return dirAddr + sizeof(directoryData) + (sizeof(directorySibling) * data);
}

/**
 * @brief add a sibling to a directory. 
 * 
 * @param dirAddr the address of the directory to add the sibling to. 
 * @param sibling the sibling to add to the directory.
 * 
 */
void FileSystem::addSibling(address dirAddr, dirSibling sibling)
{
    directoryData data;

    m_disk->read(dirAddr, sizeof(directoryData), (char*)&data);
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
void FileSystem::createCurrAndPrevDir(unsigned int currentDirInode, unsigned int prevDirInode)
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
    
    addSibling(currentDir.firstAddr, prev);
    addSibling(currentDir.firstAddr, curr);
}

/**
 * @brief convert file path to inode. 
 * 
 * @param path the path of the file to get the inode of.
 * 
 * @return FileSystem::inode the inode of the requested file.
 */
FileSystem::inode FileSystem::pathToInode(afsPath path) const 
{
    afsPath parentPath = afsPath(path.begin(), path.end() - 1);
    address parentAddr = pathToAddr(parentPath);

    dirSibling sibling = getSiblingData(parentAddr, path[path.size() - 1]);
    
    inode siblingInode;

    m_disk->read(inodeIndexToAddr(sibling.indodeTableIndex), sizeof(inode), (char*)&siblingInode);

    return siblingInode;
}