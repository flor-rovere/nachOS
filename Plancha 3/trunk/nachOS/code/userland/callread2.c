#include "syscall.h"


int main()
{
    char *argRead1[1];
    char *argRead2[1];

    argRead1[0] = "../userland/read";
    argRead2[0] = "../userland/read";

    SpaceId read1 = Exec("../userland/read", argRead1);
    Join(read1);

    SpaceId read2 = Exec("../userland/read", argRead2);
    Join(read2);

    Halt();
}
