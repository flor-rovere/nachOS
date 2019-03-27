#include "syscall.h"

int
main(void)
{
    for (unsigned i = 0; i < 100; i++)
        Write("A", 1, ConsoleOutput);
    Write("Termino consoleA\n", 17, ConsoleOutput);
    Exit(0);
}
