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


/*

Formats the default blocks the disk need to have (super block, blocks table, inodes table and root directory)

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
    createRootDir();
}


/*

create a file or a directory in the file system.

@param path The path to create the file/directory in.
@param isDir Boolean which tells us if the file is directory or not.

*/
void FileSystem::createFile(const std::string& path, bool isDir)
{
    afsPath parsedPath = parsePath(path);
    std::string fileName = parsedPath[parsedPath.size() - 1];
    unsigned int fileBlock = getFreeBlock();


    // create inode for the file.
    inode fileInode = {
        .flags = isDir ? (1 << DIRTYPE) : FILETYPE,
        .fileSize = 0,
        .firstAddr = blockToAddr(fileBlock)
    };

    reserveDBlock(fileBlock);
    createInode(fileInode);


    if (isDir)
    {
        dirSibling curr;
        directoryData currData = { 1 };
        strncpy(curr.name, ".", sizeof(curr.name));
        curr.indodeTableIndex = m_header->inodes - 1;

        m_disk->write(getFreeDirChunkAddr(fileInode.firstAddr), sizeof(dirSibling), (const char*)&curr);
        m_disk->write(fileInode.firstAddr, sizeof(directoryData), (const char*)&currData);
    }

    uint32_t subdirAddr = pathToAddr(afsPath(parsedPath.begin(), parsedPath.end() - 1));

    dirSibling file;
    directoryData data;

    strncpy(file.name, fileName.c_str(), sizeof(file.name));
    file.indodeTableIndex = m_header->inodes - 1;

    m_disk->write(getFreeDirChunkAddr(subdirAddr), sizeof(directorySibling), (const char*)&file);

    m_disk->read(subdirAddr, sizeof(directoryData), (char*)&data);    
    m_disk->write(subdirAddr, sizeof(directoryData), (const char*)&++data);    
}

// =========== Helpers (private functions) =========== //

/*

write the superblock to the newly created disk

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

/*

create the root directory.

*/
void FileSystem::createRootDir()
{
    unsigned int rootBlock = getFreeBlock();
    directoryData data = { 1 };

    reserveDBlock(rootBlock);

    inode root = {
        .flags = 1 << InodeFlags::DIRTYPE,
        .firstAddr = rootBlock * m_disk->getBlockSize()
    };

    createInode(root);
    
    dirSibling curr;
    curr.indodeTableIndex = m_header->inodes - 1;
    strncpy(curr.name, ".", sizeof(curr.name));

    m_disk->write(getFreeDirChunkAddr(blockToAddr(rootBlock)), sizeof(dirSibling), (const char*)&curr);
    m_disk->write(blockToAddr(rootBlock), sizeof(uint16_t), (const char*)&data); // write the siblings amount at the beginning of directory.
}

/*

Find available data block to use

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

/*

Write newly created inode into the disk.

@param node The inode to write to the disk.

*/
void FileSystem::createInode(inode node)
{
    m_disk->write(inodeIndexToAddr(m_header->inodes), sizeof(node), (const char*)&node);
    m_header->inodes++;
    m_disk->write(0, sizeof(struct afsHeader), (const char*)m_header);
}

/*

reserve data block in the blocks table.

@param blockNum The number of block to reserve.

*/
void FileSystem::reserveDBlock(unsigned int blockNum)
{
    m_dblocksTable[blockNum] = true;
    m_disk->write(blockToAddr(1, blockNum), sizeof(bool), (const char*)(m_dblocksTable + blockNum));
}


/*

Convert block number into address on the disk.

@param blockNum Number of block to get the address of.
@param offset Offset in the block.

@return The address of the block + the offset.

*/
uint32_t FileSystem::blockToAddr(unsigned int blockNum, unsigned int offset) const
{
    return (blockNum * m_disk->getBlockSize()) + offset;
}

/*

Convert inode index to address in the inode table.

@param inodeIndex Index of the inode to get the address of.

@return The address of the inode in the inode table.

*/
int FileSystem::inodeIndexToAddr(int inodeIndex) const
{
    return ((1 + m_dblocksTableAmount) * m_disk->getBlockSize()) + (sizeof(inode) * inodeIndex);
}

/*

Parse path into parts (seperated by /)

@param pathStr the path to parse.

@return vector of strings with the path splitted.

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

FileSystem::inode FileSystem::getRoot() const
{
    inode root;

    m_disk->read(inodeIndexToAddr(0), sizeof(inode), (char*)&root);

    return root;
}

FileSystem::dirSibling FileSystem::getSiblingData(const uint32_t dirAddr, int indx) const
{
    dirSibling sibling;
    
    m_disk->read((dirAddr + sizeof(directoryData)) + (indx * sizeof(dirSibling)), sizeof(dirSibling), (char*)&sibling);
    
    return sibling;
}

/*
TODO: Document
*/
uint32_t FileSystem::pathToAddr(afsPath path) const
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
        
        if(!(curr.flags & (1 << DIRTYPE)))
            throw std::runtime_error("path contains file that is not a directory.");
    } 


    return curr.firstAddr;
}

uint32_t FileSystem::getFreeDirChunkAddr(uint32_t dirAddr)
{
    directoryData data;
    m_disk->read(dirAddr, sizeof(directoryData), (char*)&data);
    return dirAddr + sizeof(directoryData) + (sizeof(directorySibling) * data);
}