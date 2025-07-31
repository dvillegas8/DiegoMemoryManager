//
// Created by diego.villegas on 7/9/2025.
#include "../include/reader.h"
#include "../include/initializer.h"
void read_disk(ULONG64 disk_index, ULONG64 frameNumber, PTHREAD_INFO threadInfo) {
    PVOID diskAddress = (PVOID)((ULONG64) vmState.disk + (disk_index * PAGE_SIZE));
    if (MapUserPhysicalPages(threadInfo->transferVA, 1, &frameNumber) == FALSE) {
        DebugBreak();
        printf("read_to_disk : transfer_va could not be mapped");
    }
    memcpy(threadInfo->transferVA, diskAddress, PAGE_SIZE);
    if (MapUserPhysicalPages(threadInfo->transferVA, 1, NULL) == FALSE) {
        DebugBreak();
        printf("read_to_disk : transfer_va could not be unmapped");
    }
    // Make disk page available
    vmState.disk_pages[disk_index] = 0;
}
//
