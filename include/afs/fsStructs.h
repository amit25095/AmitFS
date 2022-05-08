#pragma once

enum InodeFlags
{
    DELETED = 1 << 0,
    FILETYPE = 1 << 1,
    DIRTYPE = 1 << 2
};

typedef struct inode
{
    int flags;
    uint32_t fileSize;
    address firstAddr;
} inode;

typedef struct directorySibling
{
    char name[NAME_MAX_LEN]; 
    uint32_t indodeTableIndex;
} dirSibling;