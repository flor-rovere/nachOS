#include "syscall.h"

int
main(void)
{
    OpenFileId ofi = Open("hola.txt");
    int size = 6;
    char buf[size + 5];
    Read(buf, size, ofi);
    Write(buf, size + 5, ConsoleOutput);
    Close(ofi);
    Exit(0);
}

