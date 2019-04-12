#ifndef NACHOS_USERPROG_SYNCHCONSOLE__HH
#define NACHOS_USERPROG_SYNCHCONSOLE__HH

#include "threads/synch.hh"
#include "machine/console.hh"

class SynchConsole {
public:
    // Inicializa la consola sincronizada
    SynchConsole(const char* readFile, const char* writeFile);
    
    // Destruye la consola sincronizada
    ~SynchConsole();

    // Escribe un caracter
    void SynchPutChar(char ch);

    // Lee un caracter
    char SynchGetChar();

    void SynchWriteDone();      // Controla que haya finalizado la escritura
    void SynchReadAvailable();  // Constrola si hay algo para leer

private:
    Console *console;           // Consola.
    Semaphore *readsem;         // Sincroniza hilos en espera de lectura con el
                                // manipulador de interrupciones.
    Semaphore *writesem;        // Sincroniza hilos en espera de escritura con el
                                // manipulador de interrupciones.
    Lock *readlock;             // Solo una peticion lectura se puede enviar a la
                                // consola al mismo tiempo.
    Lock *writelock;             // Solo una peticion escritura se puede enviar a
                                // la consola al mismo tiempo.
};

#endif
