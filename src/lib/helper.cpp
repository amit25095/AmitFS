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

size_t Helper::getCorrectSize(size_t inputSize)
{
    if (inputSize < MIN_SIZE)
        return MIN_SIZE;

    // decrement `n` (to handle the case when `n` itself
    // is a power of 2)
    inputSize = inputSize - 1;
 
    // do till only one bit is left
    while (inputSize & (inputSize - 1)) {
        inputSize = inputSize & (inputSize - 1);        // unset rightmost bit
    }
 
    // `n` is now a power of two (less than `n`)
 
    // return next power of 2
    return inputSize << 1;
}