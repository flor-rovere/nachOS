/// Data structures for managing threads.
///
/// A thread represents sequential execution of code within a program.  So
/// the state of a thread includes the program counter, the processor
/// registers, and the execution stack.
///
/// Note that because we allocate a fixed size stack for each thread, it is
/// possible to overflow the stack -- for instance, by recursing to too deep
/// a level.  The most common reason for this occuring is allocating large
/// data structures on the stack.  For instance, this will cause problems:
///
///     void foo() { int buf[1000]; ...}
///
/// Instead, you should allocate all data structures dynamically:
///
///     void foo() { int *buf = new int[1000]; ...}
///
/// Bad things happen if you overflow the stack, and in the worst case, the
/// problem may not be caught explicitly.  Instead, the only symptom may be
/// bizarre segmentation faults.  (Of course, other problems can cause seg
/// faults, so that is not a sure sign that your thread stacks are too
/// small.)
///
/// One thing to try if you find yourself with segmentation faults is to
/// increase the size of thread stack -- `STACK_SIZE`.
///
/// In this interface, forking a thread takes two steps.  We must first
/// allocate a data structure for it:
///     t = new Thread
/// Only then can we do the fork:
///     t->Fork(f, arg)
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2018 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_THREADS_THREAD__HH
#define NACHOS_THREADS_THREAD__HH


#include "lib/utility.hh"

#ifdef USER_PROGRAM
#include "machine/machine.hh"
#include "userprog/address_space.hh"
#include "userprog/syscall.h"
#define NUM_MAX_FILES 10 
#endif

class Port;

/// CPU register state to be saved on context switch.
///
/// x86 processors needs 9 32-bit registers, whereas x64 has 8 extra
/// registers.  We allocate room for the maximum of these two architectures.
const unsigned MACHINE_STATE_SIZE = 17;

/// Size of the thread's private execution stack.
///
/// In words.
///
/// WATCH OUT IF THIS IS NOT BIG ENOUGH!!!!!
const unsigned STACK_SIZE = 4 * 1024;


/// Thread state.
enum ThreadStatus {
    JUST_CREATED,
    RUNNING,
    READY,
    BLOCKED,
    NUM_THREAD_STATUS
};

/// The following class defines a “thread control block” -- which represents
/// a single thread of execution.
///
/// Every thread has:
/// * an execution stack for activation records (`stackTop` and `stack`);
/// * space to save CPU registers while not running (`machineState`);
/// * a `status` (running/ready/blocked).
///
///  Some threads also belong to a user address space; threads that only run
///  in the kernel have a `NULL` address space.
class Thread {
private:

    // NOTE: DO NOT CHANGE the order of these first two members.
    // THEY MUST be in this position for `SWITCH` to work.

    /// The current stack pointer.
    HostMemoryAddress *stackTop;

    /// All registers except for `stackTop`.
    HostMemoryAddress machineState[MACHINE_STATE_SIZE];

public:

    /// Initialize a `Thread`. callsJoin informa si se llamara a Join
    /// sobre este hilo. prior es la prioridad del hilo.
    Thread(const char *debugName, bool callsJoin, int prior);

    /// Deallocate a Thread.
    ///
    /// NOTE: thread being deleted must not be running when `delete` is
    /// called.
    ~Thread();

    /// Basic thread operations.

    /// Make thread run `(*func)(arg)`.
    void Fork(VoidFunctionPtr func, void *arg);

    /// Bloquea al llamante hasta que el hilo en cuestion termine.
    void Join();

    /// Relinquish the CPU if any other thread is runnable.
    void Yield();

    /// Put the thread to sleep and relinquish the processor.
    void Sleep();

    /// The thread is done executing.
    void Finish(int st);

    /// Check if thread has overflowed its stack.
    void CheckOverflow() const;

    void SetStatus(ThreadStatus st);

    const char *GetName() const;

    // Obtiene la bandera para el join.
    bool GetJoinFlag();
    
    // Obtiene la prioridad.
    int GetPriority();
    int GetRealPriority();

    void RestorePriority();

    void ChangePriority(int prior);

    void Print() const;

private:
    // Some of the private data for this class is listed above.

    /// Bottom of the stack.
    ///
    /// `NULL` if this is the main thread.  (If `NULL`, do not deallocate
    /// stack.)
    HostMemoryAddress *stack;

    /// Ready, running or blocked.
    ThreadStatus status;

    const char *name;

    /// Se utilizan para el Join.
    bool joinFlag;
    Port *threadPort;

    /// Mantiene la prioridad del hilo.
    int priority;
    int realPriority; 

    /// Allocate a stack for thread.  Used internally by `Fork`.
    void StackAllocate(VoidFunctionPtr func, void *arg);

#ifdef USER_PROGRAM
    /// User-level CPU register state.
    ///
    /// A thread running a user program actually has *two* sets of CPU
    /// registers -- one for its state while executing user code, one for its
    /// state while executing kernel code.
    int userRegisters[NUM_TOTAL_REGS];

    OpenFile *ofilesids[NUM_MAX_FILES];

public:

    // Save user-level register state.
    void SaveUserState();

    // Restore user-level register state.
    void RestoreUserState();

    // User code this thread is running.
    AddressSpace *space;

    // Funciones de actualizacion para los archivos abiertos
    OpenFileId AddNewFile(OpenFile *ofile);

    void CloseFile(OpenFileId ofileid);
    
    OpenFile *GetFile(OpenFileId ofileid);

#endif
};

/// Magical machine-dependent routines, defined in `switch.s`.

extern "C" {
    /// First frame on thread execution stack.
    ///
    /// 1. Enable interrupts.
    /// 2. Call `func`.
    /// 3. (When func returns, if ever) call `ThreadFinish`.
    void ThreadRoot();

    // Stop running `oldThread` and start running `newThread`.
    void SWITCH(Thread *oldThread, Thread *newThread);
}


#endif
