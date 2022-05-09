#pragma once

#include <afs/constants.h>

#include <vector>
#include <string>

#include <cstdint>

class Helper
{
public:
    static std::vector<std::string> splitString(const std::string& str, const char delim = '/');

    static address blockToAddr(uint32_t blockSize, unsigned int blockNum, unsigned int offset = 0);
    static unsigned int addrToBlock(uint32_t blockSize, address addr);
    static address getSiblingAddr(address parentAddr, unsigned int index);

    static bool isFileExist(const char* filePath);
    static int openExistingFile(const char* filePath);
    static uint32_t getCorrectSize(uint32_t inputSize);
};