#pragma once

class Helper
{
public:
    static bool isFileExist(const char* filePath);
    static int openExistingFile(const char* filePath);
    static size_t getCorrectSize(size_t inputSize); 
};