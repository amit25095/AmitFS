#include <afs/fs.h>
#include <afs/bootLoad.h>
#include <afs/helper.h>
#include <afs/constants.h>

#include <cstring>

FileSystem::FileSystem(const char* filePath, size_t blockSize, size_t nblocks)
{
    m_disk = BootLoad::load(filePath);

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

void FileSystem::setHeader()
{
    struct afsHeader header;

    strncpy(header.magic, MAGIC, sizeof(header.magic));
    header.version = CURR_VERSION;
    header.blockSize = m_disk->getBlockSize();
    header.nblocks = m_disk->getBlocksAmount();

    m_disk->write(0, sizeof(header), (const char*)&header);
}

void FileSystem::format()
{
    setHeader();
}