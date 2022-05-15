#pragma once

#include <vector>
#include <string>

#include <cstring>
#include <cstdint>

typedef std::vector<std::string> afsPath;

typedef uint16_t directoryData;
typedef uint32_t address;
constexpr char MAGIC[] = "AFS";
constexpr uint8_t CURR_VERSION = 0x01;

constexpr uint32_t MIN_SIZE = 512;
constexpr uint32_t MIN_BLOCKS_AMOUNT = 512;
constexpr int NAME_MAX_LEN = 28;
typedef struct dirListEntry
{
    dirListEntry(char* fileName, uint32_t size, bool isDir):
        fileSize(size), isDirectory(isDir)
    {
        strncpy(name, fileName, NAME_MAX_LEN);
    }
    char name[NAME_MAX_LEN];
    uint32_t fileSize;
    bool isDirectory;
} dirListEntry;

typedef std::vector<dirListEntry> dirList;

struct __attribute__((__packed__)) afsHeader
{
    char magic[3];
    uint8_t version;
    uint32_t blockSize;
    uint32_t nblocks;
    uint16_t inodes;
    uint16_t inodeBlocks;
};
