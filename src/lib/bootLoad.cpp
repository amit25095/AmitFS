#include <afs/bootLoad.h>
#include <afs/helper.h>

#include <iostream>

#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

/*

Loads the file system if exist, if not creates the fs file and sets it super block.

@param filePath path to the disk file.

@return disk object with the correct size accordint to the fs if the disk contains filesystem in it.

*/
Disk* BootLoad::load(const char* filePath)
{
    int fd = -1;
    struct afsHeader header;
    Disk* disk = nullptr;

    if (Helper::isFileExist(filePath))
    {
        Helper::openExistingFile(filePath);

        read(fd, &header, sizeof(afsHeader));
        if (strncmp(header.magic, MAGIC, sizeof(header.magic)) != 0 || header.version != CURR_VERSION)
            throw std::runtime_error("this file is not afs instance.");
    
        close(fd);

        disk = new Disk(filePath, header.blockSize, header.nblocks);
    }

    return disk;
}