#pragma once

#include <afs/disk.h>

#include <vector>

class BootLoad
{
public:
    static Disk* load(const char* filePath);
};