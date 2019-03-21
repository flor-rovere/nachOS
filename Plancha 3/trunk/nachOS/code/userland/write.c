#include "syscall.h"

int
main(void)
{
    OpenFileId ofi = Open("hola.txt");
    char buf[] = "hello world!\n";
    int size = sizeof buf;
    Write(buf, size, ofi);
    Close(ofi);
    Exit(0);
}
