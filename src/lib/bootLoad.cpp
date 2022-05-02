#include <afs/bootLoad.h>
#include <afs/helper.h>

#include <iostream>

#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

/*

Loads the file system if exist, and get the superblock data

@param filePath path to the disk file.

@return superblock (header) struct with all the data from the disk or nullptr if the file doesn't exist/

*/
struct afsHeader* BootLoad::load(const char* filePath)
{
    int fd = -1;
    struct afsHeader* header = nullptr;

    if (Helper::isFileExist(filePath))
    {
        fd = Helper::openExistingFile(filePath);
        header = new struct afsHeader;
        
        read(fd, header, sizeof(struct afsHeader));

        if (strncmp(header->magic, MAGIC, sizeof(header->magic)) != 0 || header->version != CURR_VERSION)
            throw std::runtime_error("this file is not afs instance.");
    
        close(fd);
    }

    return header;
}