//
// Created by diego.villegas on 7/9/2025.
// Look for a free disk index
#include "../include/writer.h"

#include "lists.h"
#include "../include/initializer.h"
VOID writeToDisk(PVOID context) {
    ULONG64 frameNumber;
    PVOID diskAddress;
    ULONG index;
    HANDLE events[2];
    PPFN victim;
    PTHREAD_INFO threadInfo;

    threadInfo = (PTHREAD_INFO)context;
    events[START_EVENT_INDEX] = vmState.startWriter;
    events[EXIT_EVENT_INDEX] = vmState.exitEvent;
    WaitForSingleObject(vmState.startEvent, INFINITE);
    while (TRUE){
        EnterCriticalSection(&vmState.modifiedListLock);
        if (IsListEmpty(&vmState.modifiedList)) {
            LeaveCriticalSection(&vmState.modifiedListLock);
            index = WaitForMultipleObjects (ARRAYSIZE(events), events, FALSE, INFINITE);
            if (index == EXIT_EVENT_INDEX) {
                printf("writer exit event\n");
                return;
            }
            continue;
        }
        victim = pop_page(&vmState.modifiedList);
        LeaveCriticalSection(&vmState.modifiedListLock);
        if (victim == NULL) {
            //printf("writeToDisk: could not find victim\n");
            DebugBreak();
            continue;
        }
        frameNumber = getFrameNumber(victim);
        // Temporarily map physical page to the transfer_va
        if (MapUserPhysicalPages(threadInfo->transferVA, 1, &frameNumber) == FALSE) {
            printf("write_to_disk : transfer_va is not mapped\n");
            DebugBreak();
        }
        // TODO: Make this more efficient because we just linear walk through the disk
        // Look for a disk_page that is available
        // We will start from the last used disk index
        // // Flag to see if we got a disk index
        BOOL disk_slot_found = FALSE;
        for(ULONG64 i=0; i < NUMBER_OF_DISK_PAGES; i++) {

            // Here we check to see if we found a good index!
            if (vmState.disk_pages[vmState.disk_page_index] == 0) {
                disk_slot_found = TRUE;
                break;
            }

            // Otherwise, we check the next one
            vmState.disk_page_index++;

            // Check if we are at the end of the array and wrap around
            if (vmState.disk_page_index == NUMBER_OF_DISK_PAGES + 1) vmState.disk_page_index = 1;
        }
        // Means that there is no disk page available
        if (disk_slot_found == FALSE) {
            // TODO: NOTE if you remove this our disk gets full
            MapUserPhysicalPages(threadInfo->transferVA, 1, NULL);
            victim->pte->entireFormat = 0;
            victim->status = PFN_STANDBY;
            add_entry(&vmState.standbyList, victim);
            SetEvent(vmState.finishWriter);
            //printf("Disk is full");
            //DebugBreak();
        }
        else {
            diskAddress = (PVOID)((ULONG64) vmState.disk + vmState.disk_page_index * PAGE_SIZE);
            // Copy contents from transfer va to diskAddress
            memcpy(diskAddress, threadInfo->transferVA, PAGE_SIZE);
            // Make disk page unavailable
            vmState.disk_pages[vmState.disk_page_index] = 1;
            // Unmap transfer VA
            if (MapUserPhysicalPages(threadInfo->transferVA, 1, NULL) == FALSE) {
                DebugBreak();
                printf("write_to_disk : transfer_va could not be unmapped");
            }
            victim->status = PFN_STANDBY;
            victim->pte->DiskFormat.mustBeZero = 0;
            victim->pte->DiskFormat.diskIndex = vmState.disk_page_index;
            EnterCriticalSection(&vmState.standByListLock);
            add_entry(&vmState.standbyList, victim);
            LeaveCriticalSection(&vmState.standByListLock);
            // update pte in pfn
            SetEvent(vmState.finishWriter);
        }
    }
}
//
