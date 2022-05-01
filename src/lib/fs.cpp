#include <afs/fs.h>
#include <afs/bootLoad.h>
#include <afs/helper.h>
#include <afs/constants.h>

#include <iostream>
#include <cstring>
#include <cmath>

FileSystem::FileSystem(const char* filePath, uint32_t blockSize, uint32_t nblocks)
{
    m_disk = BootLoad::load(filePath);

    blockSize = Helper::getCorrectSize(blockSize);

    if (nblocks < MIN_BLOCKS_AMOUNT)
        nblocks = MIN_BLOCKS_AMOUNT;

    if (!m_disk)
    {
        m_disk = new Disk(filePath, blockSize, nblocks);
        format();
    }
}

FileSystem::~FileSystem()
{
    delete m_disk;
}


void FileSystem::format()
{
    // calculate the amounts of blocks needed for the blocks table.
    size_t blockTableAmount = ceil((float)m_disk->getBlocksAmount() / m_disk->getBlockSize());
    uint32_t blockSize = m_disk->getBlockSize();
    m_inodeBlocks = ceil(m_disk->getBlocksAmount() / 10);

    int defaultBlocks = 1 + blockTableAmount + m_inodeBlocks; // header + blocksTable + inodes table

    setHeader();

    char* blocksTable = new char[blockSize * blockTableAmount]();
    for (int i = 0; i < defaultBlocks; i++)
        blocksTable[i] = 0x01;

    m_disk->write(1 * blockSize, blockSize, blocksTable);

    delete[] blocksTable;
}

// =========== Helpers (private functions) =========== //
void FileSystem::setHeader()
{
    struct afsHeader header;

    memcpy(header.magic, MAGIC, sizeof(header.magic));
    header.version = CURR_VERSION;
    header.blockSize = m_disk->getBlockSize();
    header.nblocks = m_disk->getBlocksAmount();

    m_disk->write(0, sizeof(header), (const char*)&header);
}

unsigned long FileSystem::blockToAddr(unsigned int blockNum, unsigned int offset)
{
    return (blockNum * m_disk->getBlockSize()) + offset;
}
