/// Copyright (c) 1992      The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

/// This program reads in a COFF format file, and outputs a flat file --
/// the flat file can then be copied directly to virtual memory and executed.
/// In other words, the various pieces of the object code are loaded at
/// the appropriate offset in the flat file.
///
/// Assumes coff file compiled with -N -T 0 to make sure it is not shared
/// text.


#include "coff_reader.h"
#include "threads/copyright.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// NOTE -- once you have implemented large files, it is ok to make this
// bigger!
#define STACK_SIZE              1024  // In bytes.
#define ReadStructOrDie(f, s)  ReadOrDie(f, (char *) &(s), sizeof (s))

static inline void
Die(const char *format, ...)
{
    assert(format != NULL);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");

    exit(1);
}

/// Read and check for error.
static void
ReadOrDie(FILE *f, char *buffer, size_t numBytes)
{
    assert(f != NULL);
    assert(buffer != NULL);

    if (fread(buffer, numBytes, 1, f) != 1)
        Die("File is too short");
}

/// Write and check for error.
static void
WriteOrDie(FILE *f, const char *buffer, size_t numBytes)
{
    assert(f != NULL);
    assert(buffer != NULL);

    if (fwrite(buffer, numBytes, 1, f) != 1)
        Die("Unable to write file");
}

/// Do the real work.
void
main(int argc, char *argv[])
{
    FILE *in, *out;
    int   top, tmp;
    char *buffer;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <coffFileName> <flatFileName>\n",
                argv[0]);
        exit(1);
    }

    // Open the object file (input).
    in = fopen(argv[1], "rb");
    if (in == NULL) {
        perror(argv[1]);
        exit(1);
    }

    // Open the flat file (output).
    out = fopen(argv[2], "wb");
    if (out == NULL) {
        perror(argv[2]);
        exit(1);
    }

    /// Load the COFF file.
    char *errorS;
    coffReaderData d;
    if (!CoffReaderLoad(&d, in, &errorS))
        Die(errorS);

    // Copy the segments in.
    coffSectionHeader *sc;
    top = 0;
    printf("Loading sections:\n");
    while ((sc = CoffReaderNextSection(&d)) != NULL) {
        CoffReaderPrintSection(sc);
        if (sc->physAddr + sc->size > top)
            top = sc->physAddr + sc->size;
        // No need to copy if `.bss`.
        if (strcmp(sc->name, ".bss") && strcmp(sc->name, ".sbss")) {
            if ((buffer = CoffReaderReadSection(sc, in, &errorS)) == NULL)
                Die(errorS);
            WriteOrDie(out, buffer, sc->size);
            free(buffer);
        }
    }

    // Put a blank word at the end, so we know where the end is!
    printf("Adding stack of size: %u\n", STACK_SIZE);
    fseek(out, top + STACK_SIZE - 4, SEEK_SET);
    tmp = 0;
    WriteOrDie(out, (const char *) &tmp, 4);

    fclose(in);
    fclose(out);
}
