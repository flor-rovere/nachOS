/// Routines to manage the overall operation of the file system.  Implements
/// routines to map from textual file names to files.
///
/// Each file in the file system has:
/// * a file header, stored in a sector on disk (the size of the file header
///   data structure is arranged to be precisely the size of 1 disk sector);
/// * a number of data blocks;
/// * an entry in the file system directory.
///
/// The file system consists of several data structures:
/// * A bitmap of free disk sectors (cf. `bitmap.h`).
/// * A directory of file names and file headers.
///
/// Both the bitmap and the directory are represented as normal files.  Their
/// file headers are located in specific sectors (sector 0 and sector 1), so
/// that the file system can find them on bootup.
///
/// The file system assumes that the bitmap and directory files are kept
/// “open” continuously while Nachos is running.
///
/// For those operations (such as `Create`, `Remove`) that modify the
/// directory and/or bitmap, if the operation succeeds, the changes are
/// written immediately back to disk (the two files are kept open during all
/// this time).  If the operation fails, and we have modified part of the
/// directory and/or bitmap, we simply discard the changed version, without
/// writing it back to disk.
///
/// Our implementation at this point has the following restrictions:
///
/// * there is no synchronization for concurrent accesses;
/// * files have a fixed size, set when the file is created;
/// * files cannot be bigger than about 3KB in size;
/// * there is no hierarchical directory structure, and only a limited number
///   of files can be added to the system;
/// * there is no attempt to make the system robust to failures (if Nachos
///   exits in the middle of an operation that modifies the file system, it
///   may corrupt the disk).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2018 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "file_system.hh"
#include "directory.hh"
#include "file_header.hh"
#include "lib/bitmap.hh"
#include "machine/disk.hh"


/// Sectors containing the file headers for the bitmap of free sectors, and
/// the directory of files.  These file headers are placed in well-known
/// sectors, so that they can be located on boot-up.
static const unsigned FREE_MAP_SECTOR = 0;
static const unsigned DIRECTORY_SECTOR = 1;

/// Initial file sizes for the bitmap and directory; until the file system
/// supports extensible files, the directory size sets the maximum number of
/// files that can be loaded onto the disk.
static const unsigned FREE_MAP_FILE_SIZE = NUM_SECTORS / BitsInByte;
static const unsigned NUM_DIR_ENTRIES = 10;
static const unsigned DIRECTORY_FILE_SIZE = sizeof (DirectoryEntry)
                                            * NUM_DIR_ENTRIES;

/// Initialize the file system.  If `format == true`, the disk has nothing on
/// it, and we need to initialize the disk to contain an empty directory, and
/// a bitmap of free sectors (with almost but not all of the sectors marked
/// as free).
///
/// If `format == false`, we just have to open the files representing the
/// bitmap and the directory.
///
/// * `format` -- should we initialize the disk?
FileSystem::FileSystem(bool format)
{
    DEBUG('f', "Initializing the file system.\n");
    if (format) {
        BitMap     *freeMap   = new BitMap(NUM_SECTORS);
        Directory  *directory = new Directory(NUM_DIR_ENTRIES);
        FileHeader *mapHeader = new FileHeader;
        FileHeader *dirHeader = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FREE_MAP_SECTOR);
        freeMap->Mark(DIRECTORY_SECTOR);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapHeader->Allocate(freeMap, FREE_MAP_FILE_SIZE));
        ASSERT(dirHeader->Allocate(freeMap, DIRECTORY_FILE_SIZE));

        // Flush the bitmap and directory `FileHeader`s back to disk.
        // We need to do this before we can `Open` the file, since open reads
        // the file header off of disk (and currently the disk has garbage on
        // it!).

        DEBUG('f', "Writing headers back to disk.\n");
        mapHeader->WriteBack(FREE_MAP_SECTOR);
        dirHeader->WriteBack(DIRECTORY_SECTOR);

        // OK to open the bitmap and directory files now.
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile   = new OpenFile(FREE_MAP_SECTOR);
        directoryFile = new OpenFile(DIRECTORY_SECTOR);

        // Once we have the files “open”, we can write the initial version of
        // each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
        freeMap->WriteBack(freeMapFile);     // flush changes to disk
        directory->WriteBack(directoryFile);

        if (DebugIsEnabled('f')) {
            freeMap->Print();
            directory->Print();

            delete freeMap;
            delete directory;
            delete mapHeader;
            delete dirHeader;
        }
    } else {
        // If we are not formatting the disk, just open the files
        // representing the bitmap and directory; these are left open while
        // Nachos is running.
        freeMapFile   = new OpenFile(FREE_MAP_SECTOR);
        directoryFile = new OpenFile(DIRECTORY_SECTOR);
    }
}

/// Create a file in the Nachos file system (similar to UNIX `create`).
/// Since we cannot increase the size of files dynamically, we have to give
/// Create the initial size of the file.
///
/// The steps to create a file are:
/// 1. Make sure the file does not already exist.
/// 2. Allocate a sector for the file header.
/// 3. Allocate space on disk for the data blocks for the file.
/// 4. Add the name to the directory.
/// 5. Store the new file header on disk.
/// 6. Flush the changes to the bitmap and the directory back to disk.
///
/// Return true if everything goes ok, otherwise, return false.
///
/// Create fails if:
/// * file is already in directory;
/// * no free space for file header;
/// * no free entry for file in directory;
/// * no free space for data blocks for the file.
///
/// Note that this implementation assumes there is no concurrent access to
/// the file system!
///
/// * `name` is the name of file to be created.
/// * `initialSize` is the size of file to be created.
bool
FileSystem::Create(const char *name, unsigned initialSize)
{
    ASSERT(name != NULL);

    Directory  *directory;
    BitMap     *freeMap;
    FileHeader *header;
    int         sector;
    bool        success;

    DEBUG('f', "Creating file %s, size %u\n", name, initialSize);

    directory = new Directory(NUM_DIR_ENTRIES);
    directory->FetchFrom(directoryFile);

    if (directory->Find(name) != -1)
        success = false;  // File is already in directory.
    else {
        freeMap = new BitMap(NUM_SECTORS);
        freeMap->FetchFrom(freeMapFile);
        sector = freeMap->Find();  // Find a sector to hold the file header.
        if (sector == -1)
            success = false;  // No free block for file header.
        else if (!directory->Add(name, sector))
            success = false;  // No space in directory.
        else {
            header = new FileHeader;
            if (!header->Allocate(freeMap, initialSize))
                success = false;  // No space on disk for data.
            else {
                success = true;
                // Everthing worked, flush all changes back to disk.
                header->WriteBack(sector);
                directory->WriteBack(directoryFile);
                freeMap->WriteBack(freeMapFile);
            }
            delete header;
        }
        delete freeMap;
    }
    delete directory;
    return success;
}

/// Open a file for reading and writing.
///
/// To open a file:
/// 1. Find the location of the file's header, using the directory.
/// 2. Bring the header into memory.
///
/// * `name` is the text name of the file to be opened.
OpenFile *
FileSystem::Open(const char *name)
{
    ASSERT(name != NULL);

    Directory *directory = new Directory(NUM_DIR_ENTRIES);
    OpenFile  *openFile = NULL;
    int        sector;

    DEBUG('f', "Opening file %s\n", name);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name);
    if (sector >= 0)
        openFile = new OpenFile(sector);  // `name` was found in directory.
    delete directory;
    return openFile;  // Return `NULL` if not found.
}

/// Delete a file from the file system.
///
/// This requires:
/// 1. Remove it from the directory.
/// 2. Delete the space for its header.
/// 3. Delete the space for its data blocks.
/// 4. Write changes to directory, bitmap back to disk.
///
/// Return true if the file was deleted, false if the file was not in the
/// file system.
///
/// * `name` is the text name of the file to be removed.
bool
FileSystem::Remove(const char *name)
{
    ASSERT(name != NULL);

    Directory  *directory;
    BitMap     *freeMap;
    FileHeader *fileHeader;
    int         sector;

    directory = new Directory(NUM_DIR_ENTRIES);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name);
    if (sector == -1) {
       delete directory;
       return false;  // file not found
    }
    fileHeader = new FileHeader;
    fileHeader->FetchFrom(sector);

    freeMap = new BitMap(NUM_SECTORS);
    freeMap->FetchFrom(freeMapFile);

    fileHeader->Deallocate(freeMap);  // Remove data blocks.
    freeMap->Clear(sector);           // Remove header block.
    directory->Remove(name);

    freeMap->WriteBack(freeMapFile);      // Flush to disk.
    directory->WriteBack(directoryFile);  // Flush to disk.
    delete fileHeader;
    delete directory;
    delete freeMap;
    return true;
}

/// List all the files in the file system directory.
void
FileSystem::List()
{
    Directory *directory = new Directory(NUM_DIR_ENTRIES);

    directory->FetchFrom(directoryFile);
    directory->List();
    delete directory;
}

/// Print everything about the file system:
/// * the contents of the bitmap;
/// * the contents of the directory;
/// * for each file in the directory:
///   * the contents of the file header;
///   * the data in the file.
void
FileSystem::Print()
{
    FileHeader *bitHeader = new FileHeader;
    FileHeader *dirHeader = new FileHeader;
    BitMap     *freeMap   = new BitMap(NUM_SECTORS);
    Directory  *directory = new Directory(NUM_DIR_ENTRIES);

    printf("Bit map file header:\n");
    bitHeader->FetchFrom(FREE_MAP_SECTOR);
    bitHeader->Print();

    printf("Directory file header:\n");
    dirHeader->FetchFrom(DIRECTORY_SECTOR);
    dirHeader->Print();

    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHeader;
    delete dirHeader;
    delete freeMap;
    delete directory;
}
