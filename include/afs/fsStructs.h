#pragma once

#include <afs/constants.h>

#include <cstring>

enum InodeFlags
{
    DELETED = 1 << 0,
    FILETYPE = 1 << 1,
    DIRTYPE = 1 << 2
};

typedef struct inode
{
    inode() {}
    inode(bool isDir):
        flags(isDir ? DIRTYPE : FILETYPE), fileSize(0), firstAddr((address)-1) {}
    
    int flags;
    uint32_t fileSize;
    address firstAddr;
} inode;

typedef struct directorySibling
{
    directorySibling(const char* fileName, uint32_t inodeIndex):
        indodeTableIndex(inodeIndex)
    {
        strncpy(name, fileName, NAME_MAX_LEN);
    }
    directorySibling() = default;
    char name[NAME_MAX_LEN]; 
    uint32_t indodeTableIndex;
} dirSibling;