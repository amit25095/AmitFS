#include <afs/helper.h>
#include <afs/fsStructs.h>

#include <iostream>
#include <stdexcept>
#include <sstream>

#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>

#include <afs/constants.h>

bool Helper::isFileExist(const char* filePath)
{
    return access(filePath, F_OK) == 0;
}

/**
 * @brief open a file in read/write mode. 
 * 
 * @param filePath the path of the file to open.
 * 
 * @return int the file descriptor of the opened file.
 */
int Helper::openExistingFile(const char* filePath)
{
    int fd = open(filePath, O_RDWR);

    if (fd == -1)
        throw std::runtime_error(std::string("open failed: ") + strerror(errno));

    return fd;
}

/**
 * @brief Converts the input to the closest power of 2. 
 * 
 * @param inputSize The input to convert.
 * 
 * @return uint32_t the input rounded to the closes power of 2.
 */
uint32_t Helper::getCorrectSize(uint32_t inputSize)
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

/**
* @brief Convert block number into address on the disk.
*
* @param blockNum Number of block to get the address of.
* @param offset Offset in the block.

* @return address The address of the block + the offset.

*/
address Helper::blockToAddr(uint32_t blockSize, unsigned int blockNum, unsigned int offset)
{
    return (blockNum * blockSize) + offset;
}

/**
 * @brief Convert block nu 
 * 
 * @param blockSize 
 * @param addr 
 * @return unsigned int 
 */
unsigned int Helper::addrToBlock(uint32_t blockSize, address addr) 
{
    return addr / blockSize; 
}

address Helper::getSiblingAddr(address parentAddr, unsigned int index)
{
    return parentAddr + sizeof(directoryData) + (sizeof(dirSibling) * index);
}

address Helper::getLastFileBlock(const Disk* disk, address fileAddr)
{
    address currentAddr = fileAddr;
    while (currentAddr != 0)
    {
        fileAddr = currentAddr;
        disk->read(currentAddr + disk->getBlockSize() - sizeof(address), sizeof(address), (char*)&currentAddr);
    }

    return fileAddr;
}

/**
 * 
 * @brief split string into parts by a delimiter. 
 * 
 * @param str the string to split
 * @param delim Delimiter to split the string by 
 * 
 * @return std::vector<std::string> vector of strings with the string splitted 
 */
std::vector<std::string> Helper::splitString(const std::string& str, const char delim)
{
	std::stringstream ss(str);
	std::string part;
	afsPath ans;

	while (std::getline(ss, part, delim))
		ans.push_back(part);

	return ans;
}