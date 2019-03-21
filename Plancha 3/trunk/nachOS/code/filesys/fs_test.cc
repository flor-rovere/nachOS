/// Simple test routines for the file system.
///
/// We implement:
/// * Copy -- copy a file from UNIX to Nachos.
/// * Print -- cat the contents of a Nachos file.
/// * Perftest -- a stress test for the Nachos file system read and write a
///   really large file in tiny chunks (will not work on baseline system!)
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2018 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "file_system.hh"
#include "lib/utility.hh"
#include "machine/disk.hh"
#include "machine/statistics.hh"
#include "threads/thread.hh"
#include "threads/system.hh"


#define TransferSize  10  // Make it small, just to be difficult.

/// Copy the contents of the UNIX file `from` to the Nachos file `to`.
void
Copy(const char *from, const char *to)
{
    ASSERT(from != NULL);
    ASSERT(to != NULL);

    FILE     *fp;
    OpenFile *openFile;
    int       amountRead, fileLength;
    char     *buffer;

    // Open UNIX file.
    if ((fp = fopen(from, "r")) == NULL) {
        printf("Copy: could not open input file %s\n", from);
        return;
    }

    // Figure out length of UNIX file.
    fseek(fp, 0, 2);
    fileLength = ftell(fp);
    fseek(fp, 0, 0);

    // Create a Nachos file of the same length.
    DEBUG('f', "Copying file %s, size %u, to file %s\n",
          from, fileLength, to);
    if (!fileSystem->Create(to, fileLength)) {  // Create Nachos file.
        printf("Copy: could not create output file %s\n", to);
        fclose(fp);
        return;
    }

    openFile = fileSystem->Open(to);
    ASSERT(openFile != NULL);

    // Copy the data in TransferSize chunks.
    buffer = new char[TransferSize];
    while ((amountRead = fread(buffer, sizeof(char),
                               TransferSize, fp)) > 0)
        openFile->Write(buffer, amountRead);
    delete [] buffer;

    // Close the UNIX and the Nachos files.
    delete openFile;
    fclose(fp);
}

/// Print the contents of the Nachos file `name`.
void
Print(const char *name)
{
    ASSERT(name != NULL);

    OpenFile *openFile;
    int       i, amountRead;
    char     *buffer;

    if ((openFile = fileSystem->Open(name)) == NULL) {
        printf("Print: unable to open file %s\n", name);
        return;
    }

    buffer = new char[TransferSize];
    while ((amountRead = openFile->Read(buffer, TransferSize)) > 0)
        for (i = 0; i < amountRead; i++)
            printf("%c", buffer[i]);
    delete [] buffer;

    delete openFile;  // close the Nachos file
    return;
}


/// Performance test
///
/// Stress the Nachos file system by creating a large file, writing it out a
/// bit at a time, reading it back a bit at a time, and then deleting the
/// file.
///
/// Implemented as three separate routines:
/// * `FileWrite` -- write the file.
/// * `FileRead` -- read the file.
/// * `PerformanceTest` -- overall control, and print out performance #'s.

static const char FILE_NAME[] = "TestFile";
static const char CONTENTS[] = "1234567890";
static const unsigned CONTENT_SIZE = sizeof CONTENTS - 1;
static const unsigned FILE_SIZE = CONTENT_SIZE * 5000;

static void
FileWrite()
{
    OpenFile *openFile;
    int       i, numBytes;

    printf("Sequential write of %u byte file, in %u byte chunks\n",
           FILE_SIZE, CONTENT_SIZE);
    if (!fileSystem->Create(FILE_NAME, 0)) {
        printf("Perf test: cannot create %s\n", FILE_NAME);
        return;
    }
    openFile = fileSystem->Open(FILE_NAME);
    if (openFile == NULL) {
        printf("Perf test: unable to open %s\n", FILE_NAME);
        return;
    }
    for (i = 0; i < FILE_SIZE; i += CONTENT_SIZE) {
        numBytes = openFile->Write(CONTENTS, CONTENT_SIZE);
        if (numBytes < 10) {
            printf("Perf test: unable to write %s\n", FILE_NAME);
            delete openFile;
            return;
        }
    }
    delete openFile;
}

static void
FileRead()
{
    OpenFile *openFile;
    char     *buffer = new char[CONTENT_SIZE];
    int       i, numBytes;

    printf("Sequential read of %u byte file, in %u byte chunks\n",
           FILE_SIZE, CONTENT_SIZE);

    if ((openFile = fileSystem->Open(FILE_NAME)) == NULL) {
        printf("Perf test: unable to open file %s\n", FILE_NAME);
        delete [] buffer;
        return;
    }
    for (i = 0; i < FILE_SIZE; i += CONTENT_SIZE) {
        numBytes = openFile->Read(buffer, CONTENT_SIZE);
        if (numBytes < 10 || strncmp(buffer, CONTENTS, CONTENT_SIZE)) {
            printf("Perf test: unable to read %s\n", FILE_NAME);
            delete openFile;
            delete [] buffer;
            return;
        }
    }
    delete [] buffer;
    delete openFile;
}

void
PerformanceTest()
{
    printf("Starting file system performance test:\n");
    stats->Print();
    FileWrite();
    FileRead();
    if (!fileSystem->Remove(FILE_NAME)) {
        printf("Perf test: unable to remove %s\n", FILE_NAME);
        return;
    }
    stats->Print();
}
