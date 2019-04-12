#include "syscall.h"

int
main(void)
{
    char *argConsA[2];
    char *argConsB[2];
    char *argConsC[2];

    argConsA[0] = "../userland/consoleA";
    argConsA[1] = 0;
    argConsB[0] = "../userland/consoleB";
    argConsB[1] = 0;
    argConsC[0] = "../userland/consoleC";
    argConsC[1] = 0;
    
    SpaceId consA = Exec("../userland/consoleA", argConsA);
    SpaceId consB = Exec("../userland/consoleB", argConsB);
    SpaceId consC = Exec("../userland/consoleC", argConsC);

    Join(consA);
    Join(consB);
    Join(consC);

    Exit(0);
}

