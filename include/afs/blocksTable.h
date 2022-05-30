#pragma once

#define DBLOCKS_TABLE_BLOCK_INDX 1

#include <afs/disk.h>
#include <afs/constants.h>

class BlocksTable
{
private:
    Disk* m_disk;
    bool* m_table;
    int m_dblocksTableAmount;

public:
    BlocksTable(Disk* disk, const bool isNew = false);
    ~BlocksTable();

    int getTableBlocksAmount() const;
    unsigned int getFreeBlock() const;

    void reserveDBlock(const unsigned int blockNum);
    void freeDBlock(const unsigned int blockNum);

    void freeAllFileBlocks(const address fileAddress);
};