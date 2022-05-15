#pragma once

#include <afs/fs.h>

class Shell
{
private:
    FileSystem fs;

public:
    Shell(int argc, char* argv[]);


};