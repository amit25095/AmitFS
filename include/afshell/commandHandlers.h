#pragma once

#include <afs/fs.h>

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

typedef std::vector<std::string> args;
typedef std::unordered_map<std::string, std::function<void(FileSystem*, args)>> handlers;

class CommandHandlers
{
private:
    static handlers handlersMap;

    static void createFile(FileSystem* fs, args argv);
    static void removeFile(FileSystem* fs, args argv);
    static void createDirectory(FileSystem* fs, args argv);
    static void listFiles(FileSystem* fs, args argv);
    static void addContent(FileSystem* fs, args argv);
    static void showContent(FileSystem* fs, args argv);
    static void changeDirectory(FileSystem* fs, args argv);

public:
    static void handleCommand(FileSystem* fs, const std::string& cmd, args argv);
};
