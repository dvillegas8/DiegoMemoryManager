//
// Created by diego.villegas on 7/23/2025.
//

#ifndef MAIN_H
#define MAIN_H
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "../macros.h"
#include "../include/initializer.h"
#include "../include/lists.h"
#include "../include/pfn.h"
#include "../include/pte.h"
#include "../include/reader.h"
#include "../include/trimmer.h"
#include "../include/util.h"
#include "../include/writer.h"
#include "../include/pagefaulthandler.h"

// Main functions
VOID malloc_test (VOID);
VOID commit_at_fault_time_test (VOID);
ULONG accessVirtualMemory (PVOID Context);
void tearDownVirtualMemory();
VOID full_virtual_memory_test (VOID);
VOID main (int argc, char** argv);

#endif //MAIN_H
