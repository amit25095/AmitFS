#include <cstdint>
#include <stddef.h>

constexpr char MAGIC[] = "AFS";
constexpr uint8_t CURR_VERSION = 0x01;
constexpr size_t MIN_SIZE = 512;
constexpr size_t MIN_BLOCKS_AMOUNT = 512;

    
struct __attribute__((__packed__)) afsHeader
{
    char magic[3];
    uint8_t version;
    size_t blockSize;
    size_t nblocks;
};