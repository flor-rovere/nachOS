#include "synch_console.hh"

static void
ConsoleWriteDone(void *arg)
{
    SynchConsole *console = (SynchConsole *) arg;
    console->SynchWriteDone();
}

void
SynchConsole::SynchWriteDone()
{
    writesem -> V();
}

static void
ConsoleReadAvailable(void *arg)
{
    SynchConsole *console = (SynchConsole *) arg;
    console->SynchReadAvailable();
}

void
SynchConsole::SynchReadAvailable()
{
    readsem -> V();
}

SynchConsole::SynchConsole(const char* readFile, const char* writeFile)
{
    console = new Console(readFile, 
                          writeFile, 
                          ConsoleReadAvailable, 
                          ConsoleWriteDone, 
                          this);

    readsem = new Semaphore("Read Semaphore", 0);
    writesem = new Semaphore("Write Semaphore", 0);

    readlock = new Lock("Read Lock");
    writelock = new Lock("Write Lock");
}

SynchConsole::~SynchConsole()
{
    delete console;
    delete readsem;
    delete writesem;
    delete readlock;
    delete writelock;
}

void
SynchConsole::SynchPutChar(char ch)
{
    writelock -> Acquire();  // Solo una escritura a la vez
    console -> PutChar(ch); // Escribo en consola
    writesem -> P();        // Espero la interrupcion
    writelock -> Release(); // Libero el lock
}

char
SynchConsole::SynchGetChar()
{
    readlock -> Acquire();           // Solo una lectura a la vez
    readsem -> P();                 // Espero la interrupcion
    char ch = console -> GetChar(); // Leo desde consola
    readlock -> Release();          // Libero el lock
    return ch;
}
