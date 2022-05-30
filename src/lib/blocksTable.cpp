#include <afs/blocksTable.h>
#include <afs/helper.h>

BlocksTable::BlocksTable(Disk* disk, const bool isNew):
    m_disk(disk)
{
    uint32_t blockSize = m_disk->getBlockSize();
    m_dblocksTableAmount = m_disk->getBlocksAmount() / blockSize;
    m_table = new bool[m_dblocksTableAmount * blockSize]{false};

    if (!isNew)
    {
        for (int i = 0; i < m_dblocksTableAmount; i++)
        {
            address addr = Helper::blockToAddr(blockSize, DBLOCKS_TABLE_BLOCK_INDX, i * blockSize);
            m_disk->read(addr, blockSize, (char*)(m_table + (i * blockSize)));
        }
    }
}

BlocksTable::~BlocksTable()
{
    delete[] m_table;
}

/**
* @brief Find available data block to use
*
* @return unsigned int The number of the found block.
*/
unsigned int BlocksTable::getFreeBlock() const
{
    unsigned int i = 0;
    bool found = false;

    for (i = 0; i < m_dblocksTableAmount * m_disk->getBlockSize() && !found; i++)
    {
        if (!m_table[i])
            found = true;
    }

    return found ? i - 1 : -1;
}

int BlocksTable::getTableBlocksAmount() const
{
    return m_dblocksTableAmount;
}

/**
* @brief reserve data block in the blocks table.
*
* @param blockNum The number of block to reserve.
*/
void BlocksTable::reserveDBlock(const unsigned int blockNum)
{
    m_table[blockNum] = true;
    m_disk->write(Helper::blockToAddr(m_disk->getBlockSize(), 1, blockNum), sizeof(bool), (const char*)(m_table + blockNum));
}

/**
 * @brief release a data block. 
 * 
 * @param blockNum The number of block to release.
 */
void BlocksTable::freeDBlock(const unsigned int blockNum)
{
    m_table[blockNum] = false;
    m_disk->write(Helper::blockToAddr(m_disk->getBlockSize(), 1, blockNum), sizeof(bool), (const char*)(m_table + blockNum));
}

void BlocksTable::freeAllFileBlocks(const address fileAddr)
{
    address currentAddr = fileAddr, prevAddr;
    uint32_t blockSize = m_disk->getBlockSize();
    const char reset[sizeof(address)] = { 0 };

    while (currentAddr != 0)
    {
        prevAddr = currentAddr;
        freeDBlock(Helper::addrToBlock(blockSize, currentAddr));
        m_disk->read(currentAddr + blockSize - sizeof(address), sizeof(address), (char*)&currentAddr);
        m_disk->write(prevAddr + blockSize - sizeof(address), sizeof(address), reset);
    }
}