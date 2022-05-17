#include <iostream>
#include <afshell/shell.h>

int main(int argc, char* argv[])
{
    Shell shell(argc, argv);

    shell.interactiveShell();

    return 0;
}