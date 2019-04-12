/// Routines to manage address spaces (executing user programs).
///
/// In order to run a user program, you must:
///
/// 1. Link with the `-N -T 0` option.
/// 2. Run `coff2noff` to convert the object file to Nachos format (Nachos
///    object code format is essentially just a simpler version of the UNIX
///    executable object code format).
/// 3. Load the NOFF file into the Nachos file system (if you have not
///    implemented the file system yet, you do not need to do this last
///    step).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "threads/system.hh"
#include "machine/machine.hh"

/// Do little endian to big endian conversion on the bytes in the object file
/// header, in case the file was generated on a little endian machine, and we
/// are re now running on a big endian machine.
static void
SwapHeader(noffHeader *noffH)
{
    ASSERT(noffH != NULL);

    noffH->noffMagic              = WordToHost(noffH->noffMagic);
    noffH->code.size              = WordToHost(noffH->code.size);
    noffH->code.virtualAddr       = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr        = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size          = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr   = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr    = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size        = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr =
      WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr  = WordToHost(noffH->uninitData.inFileAddr);
}

/// Create an address space to run a user program.
///
/// Load the program from a file `executable`, and set everything up so that
/// we can start executing user instructions.
///
/// Assumes that the object code file is in NOFF format.
///
/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
///
/// * `executable` is the file containing the object code to load into
///   memory.
AddressSpace::AddressSpace(OpenFile *exec, int pid)
{
    executable = exec;
    ASSERT(executable != NULL);
    unsigned   size;

    asid = pid;
    #ifdef VMEM
    char swapName[8];

    DEBUG('c', "Inicializando swap\n");

    sprintf(swapName, "SWAP.%d", asid);
    ASSERT(fileSystem -> Create(swapName, 0));
    swap = fileSystem -> Open(swapName);
    
    DEBUG('c', "Inicializando space\n");
    #endif

    executable->ReadAt((char *) &noffH, sizeof noffH, 0);
    if (noffH.noffMagic != NOFF_MAGIC &&
          WordToHost(noffH.noffMagic) == NOFF_MAGIC)
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFF_MAGIC);

    // How big is address space?

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size
           + USER_STACK_SIZE;
      // We need to increase the size to leave room for the stack.
    numPages = divRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;

    ASSERT(numPages <= bitMap -> NumClear());
      // Check we are not trying to run anything too big -- at least until we
      // have virtual memory.

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.

    pageTable = new TranslationEntry[numPages];
    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
          // For now, virtual page number = physical page number.
        #ifdef USE_DL
        pageTable[i].valid        = false;
        #else
        pageTable[i].physicalPage = bitMap -> Find();
        pageTable[i].valid        = true;
        #endif
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
    }
    
    states = new States[numPages];
    for (unsigned i = 0; i < numPages; i++)
        states[i] = IN_TLB;

    #ifndef USE_DL
    // Zero out the entire address space, to zero the unitialized data
    // segment and the stack segment.
    for (unsigned i = 0; i < numPages; i++) {
        memset(&(machine->mainMemory[pageTable[i].physicalPage*PAGE_SIZE]), 0, PAGE_SIZE);
    }

    // Then, copy in the code and data segments into memory.
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%X, size %u\n",
              noffH.code.virtualAddr, noffH.code.size);

        int vaddr, ppage, offset;
        for (unsigned i = 0; i < noffH.code.size; i++) {
            vaddr = (noffH.code.virtualAddr + i);
            ppage = pageTable[vaddr / PAGE_SIZE].physicalPage;
            offset = vaddr % PAGE_SIZE;
            executable -> ReadAt(&(machine -> mainMemory[ppage * PAGE_SIZE + offset]), 1, noffH.code.inFileAddr + i);
        }
        //executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
        //                   noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%X, size %u\n",
              noffH.initData.virtualAddr, noffH.initData.size);
         
        int vaddr, ppage, offset;
        for (unsigned i = 0; i < noffH.initData.size; i++) {
            vaddr = (noffH.initData.virtualAddr + i);
            ppage = pageTable[vaddr / PAGE_SIZE].physicalPage;
            offset = vaddr % PAGE_SIZE;
            executable -> ReadAt(&(machine -> mainMemory[ppage * PAGE_SIZE + offset]), 1, noffH.initData.inFileAddr + i);
        }
        //executable->ReadAt(
        //  &(machine->mainMemory[noffH.initData.virtualAddr]),
        //  noffH.initData.size, noffH.initData.inFileAddr);
    }
    #endif

}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
    for (unsigned i = 0; i < numPages; i++)
    {
        #ifdef VMEM
        coreMap -> Clear(pageTable[i].physicalPage);
        #endif
        bitMap -> Clear(pageTable[i].physicalPage);
    }
    delete [] pageTable;
    delete [] states;
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void
AddressSpace::SaveState()
{
    #ifdef USE_TLB
    DEBUG('b', "Guardando estado de la tlb\n");
    for (unsigned i = 0; i < TLB_SIZE; i++)
    {
        if (machine -> tlb[i].valid) //TODO: && machine -> tlb[i].dirty?
            pageTable[machine -> tlb[i].virtualPage] = machine -> tlb[i];
    }
    #endif
}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void
AddressSpace::RestoreState()
{
    #ifdef USE_TLB
    DEBUG('b', "Restituyendo estado de la tlb\n");
    for (unsigned i = 0; i < TLB_SIZE; i++)
        machine -> tlb[i].valid = false;
    #else
    DEBUG('b', "Restituyendo estado\n");
    machine->pageTable     = pageTable;
    machine->pageTableSize = numPages;
    #endif
}

TranslationEntry *
AddressSpace::GetPT(int page)
{
    return &pageTable[page]; 
}

unsigned 
AddressSpace::GetNumPages()
{
    return numPages;
}

bool 
AddressSpace::VPNControl(unsigned vpn)
{
    bool cond = vpn >=0 && vpn < numPages;
    return cond;
}

void
AddressSpace::LoadVPNFromBinary(unsigned vpn, int physPage)
{
    for (unsigned vaddr = vpn * PAGE_SIZE, i = 0; vaddr < (vpn + 1) * PAGE_SIZE; vaddr++, i++)
    {
        char c;
        unsigned offset;
        if (noffH.code.virtualAddr <= vaddr && vaddr < noffH.code.virtualAddr + noffH.code.size)
        {
            offset = vaddr - noffH.code.virtualAddr;
            executable -> ReadAt(&c, 1, noffH.code.inFileAddr + offset);
        }
        else if (noffH.initData.virtualAddr <= vaddr && vaddr < noffH.initData.virtualAddr + noffH.initData.size)
        {
            offset = vaddr - noffH.initData.virtualAddr;
            executable -> ReadAt(&c, 1, noffH.initData.inFileAddr + offset);
        }
        machine -> mainMemory[physPage * PAGE_SIZE + i] = c;
    }
}

void
AddressSpace::InsertTLB(unsigned vpn)
{
    DEBUG('b', "Ingresando a insertTLB\n");
    TranslationEntry *pT = GetPT(vpn);
    int saved = 0, newval, physPage;

    if (! pT -> valid)
    {
        #ifdef VMEM
        physPage = coreMap -> Find(this, vpn);
        #else
        physPage = bitMap -> Find();
        #endif

        DEBUG('c', "Modificando physPage por %d\n", physPage);

        if (states[vpn] == IN_SWAP)
        {
            LoadFromSwap(vpn, physPage);
            states[vpn] = IN_TLB;
        }
        else
            LoadVPNFromBinary(vpn, physPage);
    
        pT -> physicalPage = physPage;
        pT -> valid        = true;
    }

    // Buscamos una pagina invalida para agregar la informacion.
    // Si no hay ninguna, pisamos una al azar
    for (unsigned i = 0; i < TLB_SIZE; i++)
    {   
        if (! machine -> tlb[i].valid)
        {
            newval = i;
            saved  = 1;
            break;
        }
    }
    if (!saved){
        newval = rand() % TLB_SIZE; 
        //pageTable[machine->tlb[newval].virtualPage] = machine->tlb[newval]; //TODO: dejar?
    }

    // Modificamos el valor de la tlb definida en machine para
    // agregar la informacion de la nueva vpn
    DEBUG('b', "Modificando la tlb\n");
    machine -> tlb[newval].virtualPage  = pT -> virtualPage;
    machine -> tlb[newval].physicalPage = pT -> physicalPage;
    machine -> tlb[newval].valid        = pT -> valid;
    machine -> tlb[newval].readOnly     = pT -> readOnly;
    machine -> tlb[newval].use          = pT -> use;
    machine -> tlb[newval].dirty        = pT -> dirty;
    DEBUG('b', "Termino la modificacion de la tlb\n");
}

void
AddressSpace::SaveToSwap(unsigned vpn)
{
    DEBUG('c', "Guardando %d a swap\n", vpn);
    unsigned ppn = pageTable[vpn].physicalPage;
    swap -> WriteAt(&machine -> mainMemory[ppn * PAGE_SIZE], PAGE_SIZE, vpn * PAGE_SIZE);
    
    #ifdef USE_TLB
    DEBUG('c', "Invalidando la tlb\n");
    for (unsigned i = 0; i < TLB_SIZE; i++)
    {
        if (machine -> tlb[i].valid && machine -> tlb[i].virtualPage == vpn)
            machine -> tlb[i].valid = false;
    }
    #endif
    
    states[vpn] = IN_SWAP;
    //pageTable[vpn].valid = false; //TODO: borrar?
}

void
AddressSpace::LoadFromSwap(unsigned vpn, int physPage)
{
    DEBUG('c', "Buscando %d de swap\n", vpn);
    int inFileAddr = vpn * PAGE_SIZE;
    int physAddr = physPage * PAGE_SIZE;
    swap -> ReadAt(&machine -> mainMemory[physAddr], PAGE_SIZE, inFileAddr);
}
