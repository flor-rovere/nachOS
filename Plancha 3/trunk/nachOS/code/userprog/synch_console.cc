#include "synch_console.hh"
#include "threads/system.hh" //CHEQUEAR: borrar si se borra currentthread abajo

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
    DEBUG('c', "Synchputchar antes acquire en thread %s \n", currentThread->GetName()); //CHEQUEAR: borrar
    writelock -> Acquire();  // Solo una escritura a la vez
    DEBUG('c', "Synchputchar despues acquire en thread %s \n", currentThread->GetName()); //CHEQUEAR: borrar
    console -> PutChar(ch); // Escribo en consola
    writesem -> P();        // Espero la interrupcion
    DEBUG('c', "Synchputchar antes release en thread %s \n", currentThread->GetName()); //CHEQUEAR: borrar
    writelock -> Release(); // Libero el lock
    DEBUG('c', "Synchputchar despues release en thread %s \n", currentThread->GetName()); //CHEQUEAR: borrar
}

char
SynchConsole::SynchGetChar()
{
    DEBUG('c', "Synchgetchar antes acquire en thread %s \n", currentThread->GetName()); //CHEQUEAR: borrar
    readlock -> Acquire();           // Solo una lectura a la vez
    DEBUG('c', "Synchgetchar despues acquire en thread %s \n", currentThread->GetName()); //CHEQUEAR: borrar
    readsem -> P();                 // Espero la interrupcion
    char ch = console -> GetChar(); // Leo desde consola
    DEBUG('c', "Synchgetchar antes release en thread %s \n", currentThread->GetName()); //CHEQUEAR: borrar
    readlock -> Release();          // Libero el lock
    DEBUG('c', "Synchgetchar despues release en thread %s \n", currentThread->GetName()); //CHEQUEAR: borrar
    return ch;
}
