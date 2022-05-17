#include <afshell/commandHandlers.h>
#include <afshell/colors.h>

#include <iomanip> // for setw
#include <stdexcept>
#include <iostream>
#include <limits>

handlers CommandHandlers::handlersMap = {
    {"touch", CommandHandlers::createFile},
    {"rm", CommandHandlers::removeFile},
    {"mkdir", CommandHandlers::createDirectory},
    {"ls", CommandHandlers::listFiles},
    {"cat", CommandHandlers::showContent},
    {"edit", CommandHandlers::addContent},
};

void CommandHandlers::handleCommand(FileSystem* fs, const std::string& cmd, args argv)
{
    if (cmd != "exit" && cmd != "EXIT")
    {
        if (handlersMap.find(cmd) == handlersMap.end())
            throw std::runtime_error("Invalid command");

        handlersMap[cmd](fs, argv);
    }
}

void CommandHandlers::createFile(FileSystem* fs, args argv)
{
    if (argv.empty())
        throw std::runtime_error("File name was not provided!");
    
    fs->createFile(argv[0]);
}

void CommandHandlers::removeFile(FileSystem* fs, args argv)
{
    if (argv.empty())
        throw std::runtime_error("File name was not provided!");

    fs->deleteFile(argv[0]);
}

void CommandHandlers::createDirectory(FileSystem* fs, args argv)
{
    if (argv.empty())
        throw std::runtime_error("File name was not provided!");
    
    fs->createFile(argv[0], true);
}

void CommandHandlers::addContent(FileSystem* fs, args argv)
{
    std::string content = "", line;
    int lineNumber = 0;

    if (argv.empty())
        throw std::runtime_error("File name was not provided!");

    std::cout << fs->getContent(argv[0]);

    while (std::getline(std::cin, line) && (line.length() != 0 || lineNumber == 0))
    {
        content += line + "\n";
        lineNumber++;
    }

    content = content.substr(0, content.length() - 1);
    
    fs->appendContent(argv[0], content);
}

void CommandHandlers::listFiles(FileSystem* fs, args argv)
{
    if (argv.empty())
        throw std::runtime_error("no folder was specified");

    dirList list = fs->listDir(argv[0]);

    for (dirListEntry entry : list)
    {
        std::string fileType = entry.isDirectory ? "directory" : "file";
        std::string color = entry.isDirectory ? bold + blue : white;

        std::cout << cyan  << std::setw(15) << std::left << fileType <<
                     green << std::setw(10) << std::left << entry.fileSize <<
                     color << std::setw(27) << std::left << entry.name << reset << "\n";
                     
    }
}

void CommandHandlers::showContent(FileSystem* fs, args argv)
{
    if (argv.empty())
        throw std::runtime_error("File name was not specified");
    
    std::string content = fs->getContent(argv[0]);
    std::cout << content;

    if (!content.empty())
        std::cout << '\n';
}