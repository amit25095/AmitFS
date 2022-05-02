#pragma once

#include <cstdint>

constexpr char MAGIC[] = "AFS";
constexpr uint8_t CURR_VERSION = 0x01;
constexpr uint32_t MIN_SIZE = 512;
constexpr uint32_t MIN_BLOCKS_AMOUNT = 512;
constexpr int NAME_MAX_LEN = 28;
    
struct __attribute__((__packed__)) afsHeader
{
    char magic[3];
    uint8_t version;
    uint32_t blockSize;
    uint32_t nblocks;
    uint16_t inodes;
    uint16_t inodeBlocks;
};