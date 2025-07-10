//
// Created by diego.villegas on 7/9/2025.
// Look for a free disk index
#include "../include/writer.h"
#include "../include/initializer.h"
boolean write_to_disk(ULONG64 frameNumber) {
    // Temporarily map physical page to the transfer_va
    if (MapUserPhysicalPages(transfer_va, 1, &frameNumber) == FALSE) {
        DebugBreak();
        printf("write_to_disk : transfer_va is not mapped\n");
    }
    ULONG64 counter = 0;
    // TODO: Make this more efficient because we just linear walk through the disk
    // Look for a disk_page that is available
    while(disk_pages[disk_page_index] != 0 && counter != 2) {
        disk_page_index++;
        // Check if we are at the end of the array
        if (disk_page_index == (disk_size / PAGE_SIZE) - 1){
            // Wrap around the disk
            disk_page_index = 1;
            counter++;
        }
    }
    // Means that there is no disk page available
    if (counter >= 2) {
        MapUserPhysicalPages(transfer_va, 1, NULL);
        return FALSE;
    }
    PVOID diskAddress = (PVOID)((ULONG64) disk + disk_page_index * PAGE_SIZE);
    // Copy contents from transfer va to diskAddress
    memcpy(diskAddress, transfer_va, PAGE_SIZE);
    // Make disk page unavailable
    disk_pages[disk_page_index] = 1;
    // Unmap transfer VA
    if (MapUserPhysicalPages(transfer_va, 1, NULL) == FALSE) {
        DebugBreak();
        printf("write_to_disk : transfer_va could not be unmapped");
    }
    return TRUE;
}
//
