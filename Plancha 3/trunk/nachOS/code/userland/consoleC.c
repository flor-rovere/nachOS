#include "syscall.h"

int
main(void)
{
    for (unsigned i = 0; i < 100; i++)
        Write("C", 1, ConsoleOutput);
    Write("Termino consoleC\n", 17, ConsoleOutput);
    Exit(0);
}
