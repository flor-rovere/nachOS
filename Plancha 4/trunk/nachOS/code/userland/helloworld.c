#include "syscall.h"

int
main(void)
{
    Write("Hello world\n", 12, ConsoleOutput);
    Exit(0);
}

