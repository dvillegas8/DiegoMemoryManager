//
// Created by diego.villegas on 7/9/2025.
#include "../include/reader.h"
#include "../include/initializer.h"
void read_disk(ULONG64 disk_index, ULONG64 frameNumber) {
    PVOID diskAddress = (PVOID)((ULONG64) disk + (disk_index * PAGE_SIZE));
    if (MapUserPhysicalPages(transfer_va, 1, &frameNumber) == FALSE) {
        DebugBreak();
        printf("read_to_disk : transfer_va could not be mapped");
    }
    memcpy(transfer_va, diskAddress, PAGE_SIZE);
    if (MapUserPhysicalPages(transfer_va, 1, NULL) == FALSE) {
        DebugBreak();
        printf("read_to_disk : transfer_va could not be unmapped");
    }
    // Make disk page available
    disk_pages[disk_index] = 0;
}
//
