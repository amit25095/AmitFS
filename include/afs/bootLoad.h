#pragma once

#include <afs/constants.h>

#include <vector>

class BootLoad
{
public:
    static struct afsHeader* load(const char* filePath);
};