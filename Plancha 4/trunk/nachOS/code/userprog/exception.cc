/// Entry point into the Nachos kernel from user programs.
///
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "syscall.h"
#include "threads/system.hh"
#include "machine/machine.hh"
#include "args.cc"

#define MAX_NAME 128

/// Entry point into the Nachos kernel.  Called when a user program is
/// executing, and either does a syscall, or generates an addressing or
/// arithmetic exception.
///
/// For system calls, the following is the calling convention:
///
/// * system call code in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the pc before returning. (Or else you will
/// loop making the same system call forever!)
///
/// * `which` is the kind of exception.  The list of possible exceptions is
///   in `machine.hh`.

/// Funciones que copian datos desde el núcleo al espacio de memoria virtual
/// del usuario y viceversa

void
ReadStringFromUser(int userAddress, char *outString, unsigned maxByteCount)
{
    unsigned i = 0;
    int c;
    do {
        if(!machine -> ReadMem(userAddress + i, 1, &c))
            ASSERT(machine -> ReadMem(userAddress + i, 1, &c));
        outString[i++] = (char) c;
    } while (i < maxByteCount && c != '\0');
}

void
ReadBufferFromUser(int userAddress, char *outBuffer, unsigned byteCount)
{
    int c;
    for (unsigned i = 0; i < byteCount; i++){
        if(!machine -> ReadMem(userAddress + i, 1, &c))
            ASSERT(machine -> ReadMem(userAddress + i, 1, &c));
        outBuffer[i] = (char) c;
    }
}

void
WriteStringToUser(const char *string, int userAddress)
{
    unsigned i = 0;
    char c;
    do {
        c = string[i];
        if(!machine -> WriteMem(userAddress + i, 1, c))
            ASSERT(machine -> WriteMem(userAddress + i, 1, c));
        i++;
    } while (c != '\0');
}

void
WriteBufferToUser(const char *buffer, int userAddress, unsigned byteCount)
{
    char c;
    for (unsigned i = 0; i < byteCount; i++){
        c = buffer[i];
        if(!machine -> WriteMem(userAddress + i, 1, c))
            ASSERT(machine -> WriteMem(userAddress + i, 1, c));
    }
}

/// Funcion que actualiza el pc
void
IncrementPC()
{
    int pc;
    pc = machine -> ReadRegister(PC_REG);
    machine -> WriteRegister(PREV_PC_REG, pc);
    pc = machine -> ReadRegister(NEXT_PC_REG);
    machine -> WriteRegister(PC_REG, pc);
    pc += 4;
    machine -> WriteRegister(NEXT_PC_REG, pc);
}

/// Funcion con la que haremos el fork en la excepcion SC_Exec
void
ProcessCreator(void *args) 
{
    currentThread -> space -> InitRegisters();
    currentThread -> space -> RestoreState();
    WriteArgs((char **) args);
    machine -> Run();
}

/// Maneja las interrupciones
void
HandException(int type)
{
    switch (type)
    {
        case SC_Halt:
        {
            // void Halt();
            DEBUG('a', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;
        }
        case SC_Exit:
        {
            // void Exit(int status);
            int st = machine -> ReadRegister(4);
            if (st == 0)
                DEBUG('e', "Normal exit.\n");
            else
                DEBUG('e', "ERROR: User program not having a normal exit. Value: %i\n", st);
            currentThread -> Finish(st);
            break;
        }
        case SC_Exec:
        {
            // SpaceId Exec(char *name, char **argv);
            int name = machine -> ReadRegister(4);
            int argv = machine -> ReadRegister(5);
            
            char outname[MAX_NAME];
            ReadStringFromUser(name, outname, MAX_NAME);
            OpenFile *exe = fileSystem -> Open(outname);
            if (exe)
            {
                DEBUG('a', "Realizando exec a %s\n", outname);
                Thread *t = new Thread(strdup(outname), true, 0); 
                SpaceId spid = AddThread(t); 
                AddressSpace *space = new AddressSpace(exe, spid);
                t -> space = space;
                t -> Fork(ProcessCreator, SaveArgs(argv));
                machine -> WriteRegister(2, GetSpId(t));
            }
            else
            {
                DEBUG('a', "ERROR: No se puede realizar exec a %s\n", outname);
                machine -> WriteRegister(2, -1);
            }
            break;
        }
        case SC_Join:
        {
            // int Join(SpaceId id);
            SpaceId spid = machine -> ReadRegister(4);
            Thread *t = GetThread(spid);
            if (t)
            { 
                DEBUG('a', "Realizando join de %s\n", t -> GetName());
                t -> Join();
                machine -> WriteRegister(2, 0); 
                RemoveThread(spid);
            }
            else
            {
                DEBUG('a', "ERROR: No se puede realizar join.\n");
                machine -> WriteRegister(2, -1);
            }
            break;
        }
        case SC_Create:
        {
            /// void Create(char *name);
            int reg = machine -> ReadRegister(4);
            char name[MAX_NAME];
            ReadStringFromUser(reg, name, MAX_NAME);
            if (fileSystem -> Create(name,0))
                DEBUG('a', "File %s created\n", name);
            else
                DEBUG('a', "ERROR: File %s was not created.\n", name);
            break;
        }
        case SC_Open:
        {
            // OpenFileId Open(char *name);
            int name = machine -> ReadRegister(4);
            char outname[MAX_NAME];
            ReadStringFromUser(name, outname, MAX_NAME);
            OpenFile *exe = fileSystem -> Open(outname);
            if (exe)
            {
                DEBUG('a', "Abriendo archivo %s\n", outname);
                OpenFileId ofileid = currentThread -> AddNewFile(exe);
                machine -> WriteRegister(2, ofileid);
            }
            else
            {
                DEBUG('a', "ERROR: No se pudo abrir el archivo %s\n", outname);
                machine -> WriteRegister(2, -1);
            }
            break;
        }
        case SC_Read:
        {
            // int Read(char *buffer, int size, OpenFileId id);
            int buf = machine -> ReadRegister(4);
            int size = machine -> ReadRegister(5);
            OpenFileId fid = (OpenFileId) machine -> ReadRegister(6);
            char info[size];
            if (fid == ConsoleInput) 
            {
                unsigned numRead = 0;
                DEBUG('a', "Leyendo desde la consola\n");
                for (int i = 0; i < size; i++)
                {
                    info[i] = synchConsole -> SynchGetChar();
                    if (info[i] != '\n' && info[i] != '\0')
                        numRead++;
                    else
                    {
                        numRead++;
                        break;
                    }
                }
                WriteBufferToUser(info, buf, numRead); 
                machine -> WriteRegister(2, size);
            }
            else if (fid == ConsoleOutput)
                DEBUG('a', "ERROR: Leyendo desde la salida\n");
            else 
            {
                OpenFile *ofile = currentThread -> GetFile(fid);
                if (ofile){
                    DEBUG('a', "Leyendo desde archivo con id: %d\n", fid);
                    int numRead = ofile -> Read(info, size);
                    WriteBufferToUser(info, buf, numRead);
                    machine -> WriteRegister(2, numRead);
                }
                else{
                    DEBUG('a', "ERROR: Leyendo desde un archivo erroneo con id: %d\n", fid);
                    machine -> WriteRegister(2, -1);
                }
            }
            break;
        }
        case SC_Write:
        {
            /// void Write(char *buffer, int size, OpenFileId id);
            int buf = machine -> ReadRegister(4);
            int size = machine -> ReadRegister(5);
            OpenFileId fid = (OpenFileId) machine -> ReadRegister(6);
            char info[size];
            if (fid == ConsoleOutput) 
            {
                DEBUG('a', "Escribiendo en la consola\n");
                ReadBufferFromUser(buf, info, size);
                for (int i = 0; i < size; i++)
                    synchConsole -> SynchPutChar(info[i]);
            }
            else if (fid == ConsoleInput)
                DEBUG('a', "ERROR: Escribiendo en la entrada\n");
            else
            {
                ReadBufferFromUser(buf, info, size);
                OpenFile *ofile = currentThread -> GetFile(fid);
                if (ofile){
                    DEBUG('a', "Escribiendo en archivo con id: %d\n", fid);
                    ofile -> Write(info, size);
                }
                else
                   DEBUG('a', "ERROR: Escribiendo en archivo erroneo con id: %d\n", fid);
            }
            break;
        }
        case SC_Close:
        {
            // void Close(OpenFileId id);
            OpenFileId fid = machine -> ReadRegister(4);
            OpenFile *ofile = currentThread -> GetFile(fid);
            if (ofile){
                DEBUG('a', "Cerrando archivo erroneo con id: %d\n", fid);
                delete ofile;
                currentThread -> CloseFile(fid);
            }
            else
                DEBUG('a', "ERROR: Cerrando archivo erroneo con id: %d\n", fid);
            break;
        }
        default:
        {
            DEBUG('a', "ERROR: Unexpected exception : %d.\n", type);
            break;
        }
    }   
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if (which == SYSCALL_EXCEPTION) {
        HandException(type);
        IncrementPC();
    }
    #ifdef USE_TLB
    else if (which == PAGE_FAULT_EXCEPTION){
        DEBUG('b', "Handling page fault exception\n");
        int vaddr = machine -> ReadRegister(BAD_VADDR_REG);
        unsigned vpn = vaddr / PAGE_SIZE;
        DEBUG('b', "BAD_VADDR_REG: %i, vpn: %i\n", vaddr, vpn);

        if (!currentThread -> space -> VPNControl(vpn))
        {
            DEBUG('b', "Error en vpn: %i\n", vpn);
            currentThread->Finish(0);
        }
        else
            currentThread -> space -> InsertTLB(vpn); 
    }
    #endif
    else if (which == READ_ONLY_EXCEPTION){
        printf("Intentando escribir en una pagina de solo lectura\n");
        currentThread -> Finish(1);
    }
    else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(false);
    }
}
