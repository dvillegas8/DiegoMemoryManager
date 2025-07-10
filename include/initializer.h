//
// Created by diego.villegas on 7/9/2025.
//

#ifndef INITIALIZER_H
#define INITIALIZER_H
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "../include/pfn.h"

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


// Variables
// Virtual address...array? We can use this to find the beginning/base of our virtual address
PULONG_PTR p;
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

// Initialize Functions
void initialize_lists(PULONG_PTR physical_page_numbers, PPFN pfnArray, ULONG_PTR physical_page_count);
void initialize_disk_space();


#endif //INITIALIZER_H
