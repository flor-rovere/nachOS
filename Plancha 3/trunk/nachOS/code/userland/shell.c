#include "syscall.h"


#define MAX_LINE_SIZE  60
#define MAX_ARG_COUNT  32
#define ARG_SEPARATOR  ' '

#define NULL  ((void *) 0)

static inline unsigned
strlen(const char *s)
{
    if (s)
    {
        unsigned i;
        for (i = 0; s[i] != '\0'; i++);
        return i;
    }
    else
        return 0;
}

static inline void
WritePrompt(OpenFileId output)
{
    //static const char PROMPT[] = "--> "; CHEQUEAR: por que tira error ahora?
    char PROMPT[] = "--> ";
    Write(PROMPT, sizeof PROMPT - 1, output);
}

static inline void
//WriteError(const char *description, OpenFileId output) CHEQUEAR idem
WriteError(char *description, OpenFileId output)
{
    //static const char PREFIX[] = "Error: "; CHEQUEAR idem
    char PREFIX[] = "Error: ";
    //static const char SUFFIX[] = "\n"; CHEQUEAR idem
    char SUFFIX[] = "\n";
    //static const char EMPTYDESCR[] = "Empty description"; CHEQUEAR idem
    char EMPTYDESCR[] = "Empty description";

    Write(PREFIX, sizeof PREFIX - 1, output);
    if (description)
        Write(description, strlen(description), output);
    else
        Write(EMPTYDESCR, sizeof EMPTYDESCR - 1, output);
    Write(SUFFIX, sizeof SUFFIX - 1, output);
}

static unsigned
ReadLine(char *buffer, unsigned size, OpenFileId input)
{
    if (buffer)
    {
        unsigned i;

        for (i = 0; i < size; i++) {
            Read(&buffer[i], 1, input);
            if (buffer[i] == '\n') {
                buffer[i] = '\0';
                break;
            }
        }
        return i;
    }
    else
        return 0;
}

static int
PrepareArguments(char *line, char **argv, unsigned argvSize)
{
    // PENDIENTE: use `bool` instead of `int` as return type; for doing this,
    //            given that we are in C and not C++, it is convenient to
    //            include `stdbool.h`.
    
    if (!line || !argv || argvSize <= 0)
        return 0;

    unsigned argCount;

    argv[0] = line;
    argCount = 1;

    // Traverse the whole line and replace spaces between arguments by null
    // characters, so as to be able to treat each argument as a standalone
    // string.
    //
    // TO DO: what happens if there are two consecutive spaces?, and what
    //        about spaces at the beginning of the line?, and at the end?
    //
    // TO DO: what if the user wants to include a space as part of an
    //        argument?
    for (unsigned i = 0; line[i] != '\0'; i++)
        if (line[i] == ARG_SEPARATOR) {
            if (argCount == argvSize - 1)
                // The maximum of allowed arguments is exceeded, and
                // therefore the size of `argv` is too.  Note that 1 is
                // decreased in order to leave space for the NULL at the end.
                return 0;
            line[i] = '\0';
            argv[argCount] = &line[i + 1];
            argCount++;
        }

    argv[argCount] = NULL;
    return 1;
}

int
main(void)
{
    const OpenFileId INPUT  = ConsoleInput;
    const OpenFileId OUTPUT = ConsoleOutput;
    char             line[MAX_LINE_SIZE];
    char            *argv[MAX_ARG_COUNT];

    for (;;) {
        WritePrompt(OUTPUT);
        const unsigned lineSize = ReadLine(line, MAX_LINE_SIZE, INPUT);
        if (lineSize == 0)
            continue;

        if (PrepareArguments(line, argv, MAX_ARG_COUNT) == 0) {
            WriteError("too many arguments.", OUTPUT);
            continue;
        }

        // Comment and uncomment according to whether command line arguments
        // are given in the system call or not.
        const SpaceId newProc = Exec(line, argv);
        //const SpaceId newProc = Exec(line);
        if (newProc == -1) {
            WriteError("cannot create new thread", OUTPUT);
            continue;
        }
            
        const int joinProc = Join(newProc);
        if (joinProc == -1) {
            WriteError("proc never created", OUTPUT);
            continue;
        }
    }

    return 0;  // Never reached.
}
