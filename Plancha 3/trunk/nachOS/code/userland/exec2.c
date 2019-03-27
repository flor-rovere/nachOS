#include "syscall.h"


int main(void)
{
    char *argProgId[1];
    argProgId[0] = "../userland/prog1";
    
    SpaceId progId = Exec("../userland/prog1", argProgId);

    Write("Espe\n", 5, ConsoleOutput);
    Join(progId);

    Halt();
}
