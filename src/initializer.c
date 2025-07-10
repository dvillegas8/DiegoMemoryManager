//
// Created by diego.villegas on 7/9/2025.
#include "../include/initializer.h"
#include "../macros.h"
#include "../include/lists.h"
// Initialize both active and free list and add pfns to free list
void initialize_lists (PULONG_PTR physical_page_numbers, PPFN pfnarray, ULONG_PTR physical_page_count) {
    // Create the head of our active list
    InitializeListHead(&activeList);
    // Create the Head of our free list
    InitializeListHead(&freeList);
    // Create the head of our modified list
    InitializeListHead(&modifiedList);
    // Get the head of our free list
    PLIST_ENTRY head = &freeList;
    PPFN pfn;
    // Add all pages to free list
    for (int i = 0; i < physical_page_count; i++) {
        pfn = &pfnarray[i];
        // Save the frame number of a certain physical page into it's PFN
        pfn->frameNumber = physical_page_numbers[i];
        // Add pfn into our doubly linked list
        add_entry(head, pfn);
    }
}
void initialize_disk_space() {
    // VA space - PFN space ex: 10 virtual pages - 3 PFN pages = 7 disk pages, having 10 would be a waste
    // We want 7 in this example so that we can have enough space to do swapping
    // We add PAGE_SIZE to help us swap a disk page and physical page when all disk pages are full
    disk_size = VIRTUAL_ADDRESS_SIZE - (NUMBER_OF_PHYSICAL_PAGES * PAGE_SIZE) + PAGE_SIZE;
    disk = malloc(disk_size);
    if (disk == NULL) {
        printf("initialize_disk_space : disk_space malloc failed");
    }
    memset(disk, 0, disk_size);
    disk_pages = malloc(disk_size / PAGE_SIZE);
    if (disk_pages == NULL) {
        printf("initialize_disk_space : disk_page malloc failed");
    }
    // 0 means that the disk page is available, 1 means the disk page is in use
    memset(disk_pages, 0, disk_size / PAGE_SIZE);
    // Skip index 0 so that when we page fault, we can correctly check diskIndex in PTE invalid format
    disk_page_index = 1;
}

//
