/// Data structures to keep track of executing user programs (address
/// spaces).
///
/// For now, we do not keep any information about address spaces.  The user
/// level CPU state is saved and restored in the thread executing the user
/// program (see `thread.hh`).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_USERPROG_ADDRESSSPACE__HH
#define NACHOS_USERPROG_ADDRESSSPACE__HH


#include "filesys/file_system.hh"
#include "machine/translation_entry.hh"
#include "bin/noff.h"

const unsigned USER_STACK_SIZE = 1024;  ///< Increase this as necessary!

enum States 
{
    IN_SWAP,
    IN_TLB
};

class AddressSpace {
public:

    /// Create an address space, initializing it with the program stored in
    /// the file `executable`.
    ///
    /// * `executable` is the open file that corresponds to the program.
    AddressSpace(OpenFile *executable, int pid);

    /// De-allocate an address space.
    ~AddressSpace();

    /// Initialize user-level CPU registers, before jumping to user code.
    void InitRegisters();

    /// Save/restore address space-specific info on a context switch.

    void SaveState();
    void RestoreState();

    TranslationEntry *GetPT(int page);
    unsigned GetNumPages();

    /// Checks that the vpn is between parameters
    bool VPNControl(unsigned vpn);

    void InsertTLB(unsigned vpn);
    void LoadVPNFromBinary(unsigned vpn, int physPage);

    void SaveToSwap(unsigned vpn);
    void LoadFromSwap(unsigned vpn, int physPage);
private:

    /// Assume linear page table translation for now!
    TranslationEntry *pageTable;

    /// Number of pages in the virtual address space.
    unsigned numPages;

    OpenFile *executable;
    noffHeader noffH;

    States *states;
    OpenFile *swap;
    int asid;
};


#endif
