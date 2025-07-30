//
// Created by diego.villegas on 7/9/2025.
//

#ifndef INITIALIZER_H
#define INITIALIZER_H
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "../include/pfn.h"
#include "../include/util.h"
#include "../include/main.h"

#define PAGE_SIZE                   4096
#define MB(x)                       ((x) * 1024 * 1024)
//
// This is intentionally a power of two so we can use masking to stay
// within bounds.
//
#define VIRTUAL_ADDRESS_SIZE        MB(16)
#define VIRTUAL_ADDRESS_SIZE_IN_UNSIGNED_CHUNKS        (VIRTUAL_ADDRESS_SIZE / sizeof (ULONG_PTR))
//
// Deliberately use a physical page pool that is approximately 1% of the
// virtual address space
//
#define NUMBER_OF_PHYSICAL_PAGES   ((VIRTUAL_ADDRESS_SIZE / PAGE_SIZE) / 64)

#define DISK_SIZE_IN_BYTES       (((VIRTUAL_ADDRESS_SIZE) - ((NUMBER_OF_PHYSICAL_PAGES) * PAGE_SIZE)) + (PAGE_SIZE * 2))

#define NUMBER_OF_DISK_PAGES    ((DISK_SIZE_IN_BYTES) / (PAGE_SIZE) - 1)

// Thread information
#define DEFAULT_SECURITY        ((LPSECURITY_ATTRIBUTES) NULL)
#define DEFAULT_STACK_SIZE      0
#define DEFAULT_CREATION_FLAGS  0
#define AUTO_RESET              FALSE

#define NUM_OF_USER_THREADS     1
#define NUM_OF_TRIMMER_THREADS  1
#define NUM_OF_WRITER_THREADS   1
// Event information
#define START_EVENT_INDEX       0
#define EXIT_EVENT_INDEX        1


// Variables


// Virtual address array/base
PULONG_PTR vaBase;
// PTE array called pageTable
PPTE pageTable;
// Index that has access to a disk page that is available
ULONG64 disk_page_index;
// Array of booleans to show us which disk pages are available
boolean* disk_pages;
// Disk
PVOID disk;
// Space for disk
ULONG_PTR disk_size;
// Used to help us write to disk
PULONG_PTR transfer_va;
// PFN array
PPFN pfnarray;
// Array which holds all the frame numbers
PULONG_PTR physical_page_numbers;
ULONG_PTR virtual_address_size_in_unsigned_chunks;

// Events
HANDLE startEvent;
HANDLE startTrimmer;
HANDLE finishTrimmer;
HANDLE exitEvent;
HANDLE startWriter;
HANDLE finishWriter;

// Threads
PHANDLE userThreads;
PHANDLE trimmerThreads;
PHANDLE writerThreads;

// Locks
CRITICAL_SECTION bigLock;

// Initialize Functions
void initialize_lists(PULONG_PTR physical_page_numbers, PPFN pfnArray, ULONG_PTR physical_page_count);
void initialize_disk_space();
void initializeVirtualMemory();
void initializeEvents();
void initializeThreads();
void initializeVirtualMemory();
void initializeLocks();


#endif //INITIALIZER_H
