#pragma once

#include <afs/disk.h>

#include <vector>

class BootLoad
{
private:
    static bool isFileExist(const char* filePath);

    struct __attribute__((__packed__)) afsHeader
    {
        char magic[3];
        int version;
        size_t blockSize;
        size_t nblocks;
    };

    inline static const char MAGIC[] = "AFS";
    inline static const int CURR_VERSION = 0x01;

public:
    static Disk* load(const char* filePath);
};