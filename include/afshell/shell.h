#pragma once

#include <afs/fs.h>

#include <vector>
#include <string>

typedef std::vector<std::string> command;

class Shell
{
private:
    FileSystem* m_fs = nullptr;
    command parseCommand(const std::string& cmd);
    void handleCommand(command cmd);

public:
    Shell(int argc, char* argv[]);
    ~Shell();


    void interactiveShell();
    
};