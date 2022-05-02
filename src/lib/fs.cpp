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

Loads the file system if exist, if not creates the fs file and sets it super block.

*/
void FileSystem::createRootDir()
{
    unsigned int rootBlock = getFreeBlock();

    inode root = {
        .flags = 1 << InodeFlags::DIRTYPE,
        .firstAddr = rootBlock * blockSize
    };

    createInode(root);
    dirSibling curr;
    curr.indodeTableIndex = m_header->inodes - 1;
    strncpy(curr.name, ".", sizeof(curr.name));

    m_disk->write(blockToAddr(rootBlock), sizeof(dirSibling), (const char*)&curr);
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

    return found ? i : -1;
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
unsigned long FileSystem::blockToAddr(unsigned int blockNum, unsigned int offset) const
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
    return ((1 + m_dblocksTableAmount) * m_disk->getBlockSize()) * (sizeof(inode) * inodeIndex);
}

