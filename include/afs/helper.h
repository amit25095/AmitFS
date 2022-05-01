#pragma once

#include <cstdint>

class Helper
{
public:
    static bool isFileExist(const char* filePath);
    static int openExistingFile(const char* filePath);
    static uint32_t getCorrectSize(uint32_t inputSize);
};