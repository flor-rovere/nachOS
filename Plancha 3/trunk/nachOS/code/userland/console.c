#include "syscall.h"

int
main(void)
{
    char *argConsA[1];
    char *argConsB[1];
    char *argConsC[1];

    argConsA[0] = "../userland/consoleA";
    argConsB[0] = "../userland/consoleB";
    argConsC[0] = "../userland/consoleC";
    
    SpaceId consA = Exec("../userland/consoleA", argConsA);
    SpaceId consB = Exec("../userland/consoleB", argConsB);
    SpaceId consC = Exec("../userland/consoleC", argConsC);

    Join(consA);
    Join(consB);
    Join(consC);

    Halt();
}

