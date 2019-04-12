#ifndef NACHOS_BIN_COFF_READER__H
#define NACHOS_BIN_COFF_READER__H


#include "coff.h"

#include <stdbool.h>
#include <stdio.h>


typedef struct coffReaderData {
    coffFileHeader fileH;
    coffOptHeader optH;
    coffSectionHeader *sections;
    unsigned current;  // Index of current section.
} coffReaderData;

bool CoffReaderLoad(coffReaderData *d, FILE *f, char **error);

void CoffReaderUnload(coffReaderData *d);

coffSectionHeader *CoffReaderNextSection(coffReaderData *d);

void CoffReaderPrintSection(const coffSectionHeader *sh);

char *CoffReaderReadSection(const coffSectionHeader *sh, FILE *f,
                            char **error);


#endif
