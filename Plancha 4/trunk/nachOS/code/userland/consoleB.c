#include "syscall.h"

int
main(void)
{
    for (unsigned i = 0; i < 100; i++)
        Write("B", 1, ConsoleOutput);
    Write("Termino consoleB\n", 17, ConsoleOutput);
    Exit(0);
}
