#include "syscall.h"

void
cat_fd(OpenFileId ofid)
{
    char *buf;
    int nread;

    while ((nread = Read(buf, sizeof buf, ofid)) > 0) 
        Write(buf, nread, ConsoleOutput);
}

void
cat(char *name)
{
    int success;
    OpenFileId ofid;
    
    if ((ofid = Open(name)) == -1)
    {
        Write("No se puede abrir el archivo.\n", 30, ConsoleOutput);
        Exit(1);
    }

    else
    {
        cat_fd(ofid);
        Close(ofid);
        Exit(0); 
    }
}

int
main(int argc, char **argv)
{
    int i;
    if (argc == 1)
        cat_fd(ConsoleInput);
    else if (argc == 2)
    {
        for (i = 1; i < argc; i++)
            cat(argv[i]);
    }
    else
    {
        Write("Error en la llamada a la funcion.\n", 34, ConsoleOutput);
        Exit(1);
    }
    Exit(0);
}
