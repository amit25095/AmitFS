#pragma once

class Helper
{
public:
    static bool isFileExist(const char* filePath);
    static int openExistingFile(const char* filePath);
};