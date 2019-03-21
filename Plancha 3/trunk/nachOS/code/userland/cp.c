#include "syscall.h"

void
cp_fd(OpenFileId ofid1, OpenFileId ofid2)
{
    int nread;
    char *buf;
    
    while ((nread = Read(buf, sizeof buf, ofid1)) > 0)  
        Write(buf, nread, ofid2);
}

void
cp(char *name1, char *name2)
{
    OpenFileId ofid1, ofid2;
    
    if ((ofid1 = Open(name1)) == -1)
    {
        Write("No se puede abrir el archivo.\n", 30, ConsoleOutput);
        Exit(1);
    }
    else
    {
        Create(name2);
        if ((ofid2 = Open(name2)) == -1)
        {
            Write("No se puede abrir el archivo.\n", 30, ConsoleOutput);
            Exit(1);
        }

        cp_fd(ofid1, ofid2);

        Close(ofid1);
        Close(ofid2);
        Exit(0);
    }
}

int
main(int argc, char **argv)
{
    if (argc == 3)
    {
        cp(argv[1], argv[2]);
        return 0;
    }
    else
    {
        Write("Error en la llamada a la funcion.\n", 34, ConsoleOutput);
        return -1;
    }
}
