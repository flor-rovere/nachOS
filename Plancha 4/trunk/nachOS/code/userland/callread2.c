#include "syscall.h"


int main()
{
    char *argRead1[2];
    char *argRead2[2];

    argRead1[0] = "../userland/read";
    argRead1[1] = 0;
    argRead2[0] = "../userland/read";
    argRead2[1] = 0;

    SpaceId read1 = Exec("../userland/read", argRead1);
    Join(read1);

    SpaceId read2 = Exec("../userland/read", argRead2);
    Join(read2);

    Exit(0);
}
