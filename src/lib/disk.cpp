#include <afs/disk.h>
#include <afs/helper.h>

#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>

Disk::Disk(const char* filePath, const size_t blockSize, const size_t nblocks):
    m_blockSize(blockSize), m_nblocks(nblocks)
{
    if (!Helper::isFileExist(filePath))
        createDiskFile(filePath);

    else
        fd = Helper::openExistingFile(filePath);

    m_fileMap = (unsigned char *)mmap(NULL, m_blockSize * m_nblocks, PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0);

	if (m_fileMap == (unsigned char *)-1)
		throw std::runtime_error(strerror(errno));
}

Disk::~Disk()
{
    munmap(m_fileMap, getDiskSize());
    close(fd);
}

void Disk::createDiskFile(const char* filePath)
{
    fd = open(filePath, O_CREAT | O_RDWR | O_EXCL, 0664);
    if (fd == -1)
        throw std::runtime_error(
            std::string("open-create failed: ") + strerror(errno));

    if (lseek(fd, getDiskSize()-1, SEEK_SET) == -1)
        throw std::runtime_error("Could not seek");

    ::write(fd, "\0", 1);
}

void Disk::read(unsigned long addr, int size, char* ans)
{
    memcpy(ans, m_fileMap + addr, size);
}

void Disk::write(unsigned long addr, int size, const char* data)
{
    memcpy(m_fileMap + addr, data, size);
}