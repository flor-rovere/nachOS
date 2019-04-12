#include "syscall.h"


int main()
{
    int i;

    Write("Ingresamos a prog1\n",19, ConsoleOutput);
    
    for (i = 1; i <= 10; i++)
    {
        Write("Estamos dentro del loop de prog1\n", 33, ConsoleOutput);
    }

    Exit(0);
}
