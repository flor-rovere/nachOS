#include "syscall.h"


int main()
{
    char *argProgId[1];
    argProgId[0] = "../userland/prog1";
    
    Write("Ejecutando prog1\n", 17, ConsoleOutput);

    SpaceId progId = Exec("../userland/prog1", argProgId);

    Write("Esperando que prog1 termine\n", 28, ConsoleOutput);
    Join(progId);

    Write("Finalizo prog1\n", 15, ConsoleOutput);

    Halt();
}
