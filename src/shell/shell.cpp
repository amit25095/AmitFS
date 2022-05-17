#include <afshell/shell.h>
#include <afshell/commandHandlers.h>

#include <iostream>
#include <sstream>

Shell::Shell(int argc, char* argv[])
{
    if (argc < 2 || (argc > 2 && argc < 4))
    {
        std::cerr << "Usage: " << argv[0] << " <disk name> [<block size> <blocks amount>]" << std::endl;
        exit(1);
    }
    
    if (argc == 2)
        m_fs = new FileSystem(argv[1]);
    else if (argc == 4)
        m_fs = new FileSystem(argv[1], std::stoi(argv[2]), std::stoi(argv[3]));
}

Shell::~Shell()
{
    if (m_fs)
    {
        delete m_fs;
        m_fs = nullptr;
    }
}

command Shell::parseCommand(const std::string& cmd)
{
	std::stringstream ss(cmd);
	std::string part, multipleWordParam;
	command ans;

    while (std::getline(ss, part, ' '))
    {
        if (part[0] == '"' || part[0] == '\'')
        {
            multipleWordParam = part.substr(1);
            std::getline(ss, part, part[0]);
            multipleWordParam += " " + part;
            ans.push_back(multipleWordParam);
        }
        else
            ans.push_back(part);
    }

	return ans;
}

void Shell::handleCommand(command cmd)
{
    std::string command = cmd[0];
    std::vector<std::string> args(cmd.begin() + 1, cmd.end());

    CommandHandlers::handleCommand(m_fs, command, args);
}

void Shell::interactiveShell()
{
    std::string cmd;

    do
    {
        std::cout << ">>> ";
        std::getline(std::cin >> std::ws, cmd);
        command parsedCmd = parseCommand(cmd);
        
        try
        {
            CommandHandlers::handleCommand(m_fs, parsedCmd[0], command(parsedCmd.begin() + 1, parsedCmd.end()));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        
    } while (cmd != "EXIT" && cmd != "exit");
}