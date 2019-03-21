/// Routines for synchronizing threads.
///
/// Three kinds of synchronization routines are defined here: semaphores,
/// locks and condition variables (the implementation of the last two are
/// left to the reader).
///
/// Any implementation of a synchronization routine needs some primitive
/// atomic operation.  We assume Nachos is running on a uniprocessor, and
/// thus atomicity can be provided by turning off interrupts.  While
/// interrupts are disabled, no context switch can occur, and thus the
/// current thread is guaranteed to hold the CPU throughout, until interrupts
/// are reenabled.
///
/// Because some of these routines might be called with interrupts already
/// disabled (`Semaphore::V` for one), instead of turning on interrupts at
/// the end of the atomic operation, we always simply re-set the interrupt
/// state back to its original value (whether that be disabled or enabled).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2018 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "synch.hh"
#include "system.hh"


/// Initialize a semaphore, so that it can be used for synchronization.
///
/// * `debugName` is an arbitrary name, useful for debugging.
/// * `initialValue` is the initial value of the semaphore.
Semaphore::Semaphore(const char *debugName, int initialValue)
{
    name  = debugName;
    value = initialValue;
    queue = new List<Thread *>;
}

/// De-allocate semaphore, when no longer needed.
///
/// Assume no one is still waiting on the semaphore!
Semaphore::~Semaphore()
{
    delete queue;
}

const char *
Semaphore::GetName() const
{
    return name;
}

/// Wait until semaphore `value > 0`, then decrement.
///
/// Checking the value and decrementing must be done atomically, so we need
/// to disable interrupts before checking the value.
///
/// Note that `Thread::Sleep` assumes that interrupts are disabled when it is
/// called.
void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);
      // Disable interrupts.

    while (value == 0) {  // Semaphore not available.
        queue->Append(currentThread);  // So go to sleep.
        currentThread->Sleep();
    }
    value--;  // Semaphore available, consume its value.

    interrupt->SetLevel(oldLevel);  // Re-enable interrupts.
}

/// Increment semaphore value, waking up a waiter if necessary.
///
/// As with `P`, this operation must be atomic, so we need to disable
/// interrupts.  `Scheduler::ReadyToRun` assumes that threads are disabled
/// when it is called.
void
Semaphore::V()
{
    Thread   *thread;
    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);

    thread = queue->Pop();
    if (thread != NULL)  // Make thread ready, consuming the `V` immediately.
        scheduler->ReadyToRun(thread);
    value++;
    interrupt->SetLevel(oldLevel);
}

/// Dummy functions -- so we can compile our later assignments.
///
/// Note -- without a correct implementation of `Condition::Wait`, the test
/// case in the network assignment will not work!

Lock::Lock(const char *debugName)
{
    name = debugName;
    semLock = new Semaphore(name, 1);
    threadLock = NULL;
}

Lock::~Lock()
{
    delete semLock;
}

const char *
Lock::GetName() const
{
    return name;
}

void
Lock::Acquire()
{
    ASSERT(!(IsHeldByCurrentThread()));
    
    int currentThreadPriority = currentThread -> GetPriority();
    if (threadLock != NULL && threadLock -> GetPriority() < currentThreadPriority){
        threadLock -> ChangePriority(currentThreadPriority);
        scheduler -> SchChangePriority(threadLock);
    }
    
    semLock -> P();
    threadLock = currentThread;
}

void
Lock::Release()
{
    ASSERT(IsHeldByCurrentThread());
    
    if (currentThread -> GetPriority() != currentThread -> GetRealPriority()){
        scheduler -> SchRestorePriority(currentThread);
    }

    threadLock = NULL;
    semLock -> V();
}

bool
Lock::IsHeldByCurrentThread() const
{
    return threadLock == currentThread;
}

Condition::Condition(const char *debugName, Lock *conditionLock)
{
    name = debugName;
    lockCond = conditionLock;
    listCond = new List<Semaphore *>;
}

Condition::~Condition()
{
    delete listCond;
}

const char *
Condition::GetName() const
{
    return name;
}

void
Condition::Wait()
{
    ASSERT(lockCond -> IsHeldByCurrentThread());
    
    //semaforo que añadimos a la lista
    Semaphore *sem = new Semaphore("ConditionSemaphore",0);
    listCond -> Append(sem);

    //libera al lock
    lockCond -> Release();

    //se bloquea hasta recibir una notificación de otro hilo
    sem -> P();
    
    // vuelve a adquirir el lock
    lockCond -> Acquire();
}

void
Condition::Signal()
{
    ASSERT(lockCond -> IsHeldByCurrentThread());

    if (!(listCond -> IsEmpty()))
        (listCond -> Pop()) -> V();
        
}

void
Condition::Broadcast()
{
    ASSERT(lockCond -> IsHeldByCurrentThread());

    while (!(listCond -> IsEmpty()))
        (listCond -> Pop()) -> V();
}

Port::Port(const char *debugName)
{
    name = debugName;
    lockPort = new Lock(name);
    sendEspera = new Condition(name,lockPort);
    receiveEspera = new Condition(name,lockPort);
    buffer = NULL;
}

Port::~Port()
{
    delete lockPort;
    delete sendEspera;
    delete receiveEspera;
}

const char *
Port::GetName() const
{
    return name;
}

bool
Port::IsBufferEmpty()
{
    return buffer == NULL;
}

void
Port::Send(int message)
{
    lockPort -> Acquire();

    // Espera hasta que no haya nada en el buffer
    while(!(IsBufferEmpty)())
        sendEspera -> Wait();

    // Copia el mensaje en el buffer
    buffer = &message;
    
    // Aviso a Receive que hay un emisor esperando
    receiveEspera -> Signal();

    lockPort -> Release();
}

void
Port::Receive(int *message)
{
    lockPort -> Acquire();
    
    // Espera hasta que haya algo en el buffer
    while(IsBufferEmpty())
        receiveEspera -> Wait();
    
    // Copio el mensaje
    *message = *buffer;
    
    // Vacio el buffer
    buffer = NULL;

    // Aviso a Send que puede enviar
    sendEspera -> Signal();

    lockPort -> Release();
}