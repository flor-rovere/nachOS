#include "syscall.h"

//void
//consoleA(void)
//{
//    while(1)
//        Write("A",1,1);
//}
//
//void
//consoleB(void)
//{
//    while(1)
//        Write("B",1,1);
//}


int
main(void)
{
    char *argConsA[1];
    char *argConsB[1];

    argConsA[0] = "../userland/consoleA";
    argConsB[0] = "../userland/consoleB";
    
    SpaceId consA = Exec("../userland/consoleA", argConsA);
    SpaceId consB = Exec("../userland/consoleB", argConsB);

    Join(consA);
    Join(consB);

    Halt();
}

