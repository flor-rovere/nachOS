#include "syscall.h"


int
main(void)
{
    SpaceId    newProc;
    OpenFileId input  = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char       prompt[2], ch, buffer[60];
    int        i;
    char       *argBuffer[1];

    prompt[0] = '-';
    prompt[1] = '-';

    while (1)
    {
        Write(prompt, 2, output);
        i = 0;
        do
            Read(&buffer[i], 1, input);
        while (buffer[i++] != '\n');

        buffer[--i] = '\0';

        if (i > 0) {
            argBuffer[0] = buffer;
            newProc = Exec(buffer, argBuffer);
            Join(newProc);
        }
    }
}
