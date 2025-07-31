//
// Created by diego.villegas on 7/9/2025.
//

#ifndef INITIALIZER_H
#define INITIALIZER_H
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "pte.h"
#include "pfn.h"

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

// Event information
#define START_EVENT_INDEX       0
#define EXIT_EVENT_INDEX        1

#define SUPPORT_MULTIPLE_VA_TO_SAME_PAGE 1

typedef struct _THREAD_INFO {
    // Helps index into its own transfer va
    ULONG ThreadNumber;
    ULONG ThreadId;
    HANDLE ThreadHandle;
    PVOID transferVA;
} THREAD_INFO, *PTHREAD_INFO;

// Variables
typedef struct VMState {
    MEM_EXTENDED_PARAMETER parameter;
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
    // PFN array
    PPFN pfnarray;
    // Array which holds all the frame numbers
    PULONG_PTR physical_page_numbers;
    ULONG_PTR virtual_address_size_in_unsigned_chunks;
    // Array of transfer vas for each thread so we don't need to waste time with locks (speed up the program)
    PVOID* userTransferVAs;
    PVOID* trimmerTransferVAs;
    PVOID* writerTransferVAs;

    // Events
    HANDLE startEvent;
    HANDLE startTrimmer;
    HANDLE finishTrimmer;
    HANDLE exitEvent;
    HANDLE startWriter;
    HANDLE finishWriter;
    // Locks
    CRITICAL_SECTION bigLock;
    CRITICAL_SECTION freeListLock;
    CRITICAL_SECTION activeListLock;
    CRITICAL_SECTION readingLock;
    CRITICAL_SECTION zeroingPageLock;
    CRITICAL_SECTION modifiedListLock;
    CRITICAL_SECTION standByListLock;

    // thread arrays
    PTHREAD_INFO userThreads;
    PTHREAD_INFO trimmerThreads;
    PTHREAD_INFO writerThreads;

    // Lists variables
    // Our doubly linked list containing all of our free pages
    LIST_ENTRY freeList;
    // Our doubly linked list containing all of our active pages
    LIST_ENTRY activeList;
    // Doubly linked list containing pages that have been trimmers and going to be written into disk
    LIST_ENTRY modifiedList;
    // Doubly linked list containing pages that have been written into disk
    LIST_ENTRY standbyList;

    // Variables
    PPFN PFN_array;

}VMState;

extern VMState vmState;

// Initialize Functions
HANDLE
CreateSharedMemorySection (
    VOID
    );
void initialize_lists(PULONG_PTR physical_page_numbers, PPFN pfnArray, ULONG_PTR physical_page_count);
void initialize_disk_space();
void initializeVirtualMemory();
void initializeEvents();
void initializeThreads();
void initializeVirtualMemory();
void initializeLocks();


#endif //INITIALIZER_H
