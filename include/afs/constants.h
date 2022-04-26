#include <cstdint>

constexpr char MAGIC[] = "AFS";
constexpr uint8_t CURR_VERSION = 0x01;

    
struct __attribute__((__packed__)) afsHeader
{
    char magic[3];
    uint8_t version;
    size_t blockSize;
    size_t nblocks;
};