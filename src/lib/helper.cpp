#include <afs/helper.h>

#include <string>
#include <stdexcept>

#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>

bool Helper::isFileExist(const char* filePath)
{
    return access(filePath, F_OK) == 0;
}

int Helper::openExistingFile(const char* filePath)
{
    int fd = open(filePath, O_RDWR);

    if (fd == -1)
        throw std::runtime_error(std::string("open failed: ") + strerror(errno));

    return fd;
}