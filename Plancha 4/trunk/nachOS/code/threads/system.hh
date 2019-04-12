/// All global variables used in Nachos are defined here.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2018 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_THREADS_SYSTEM__HH
#define NACHOS_THREADS_SYSTEM__HH


#include "thread.hh"
#include "scheduler.hh"
#include "lib/utility.hh"
#include "machine/interrupt.hh"
#include "machine/statistics.hh"
#include "machine/timer.hh"
#include "userprog/synch_console.hh"
#include "userprog/syscall.h"

#define MAX_PID_THREADS 120

/// Initialization and cleanup routines.

// Initialization, called before anything else.
extern void Initialize(int argc, char **argv);

// Cleanup, called when Nachos is done.
extern void Cleanup();


extern Thread *currentThread;               ///< The thread holding the CPU.
extern Thread *threadToBeDestroyed;         ///< The thread that just finished.
extern Scheduler *scheduler;                ///< The ready list.
extern Interrupt *interrupt;                ///< Interrupt status.
extern Statistics *stats;                   ///< Performance metrics.
extern Timer *timer;                        ///< The hardware alarm clock.
extern Thread *threadsPid[MAX_PID_THREADS]; ///< Llevamos pids para los threads
extern SpaceId AddThread(Thread *thread);   ///< Agrega thread a threadspid. devuelve el pid (-1 si hubo algun error)
extern void RemoveThread(SpaceId spid);     ///< Saca thread de threadspid
extern Thread *GetThread(SpaceId spid);     ///< Devuelve el thread con pid pid
extern SpaceId GetSpId(Thread *thread);     ///< Devuelve el pid del thread dado

#ifdef USER_PROGRAM
#include "machine/machine.hh"
extern Machine* machine;  // User program memory and registers.

#include "userprog/synch_console.hh"
extern SynchConsole* synchConsole; // Consola sincrona

#include "lib/bitmap.hh"
extern BitMap *bitMap; // BitMap para multiprogramacion
#endif

#ifdef VMEM
#include "vmem/coremap.hh"
extern CoreMap *coreMap;    //CoreMap para virtual memory
#endif

#ifdef FILESYS_NEEDED  // *FILESYS* or *FILESYS_STUB*.
#include "filesys/file_system.hh"
extern FileSystem *fileSystem;
#endif

#ifdef FILESYS
#include "filesys/synch_disk.hh"
extern SynchDisk *synchDisk;
#endif

#ifdef NETWORK
#include "network/post.hh"
extern PostOffice *postOffice;
#endif


#endif