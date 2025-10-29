//
// Created by diego.villegas on 7/9/2025.
// Look for a free disk index
#include "../include/writer.h"

#include "lists.h"
#include "../include/initializer.h"
VOID writeToDisk(PVOID context) {
    ULONG64 frameNumbers[MAXIMUM_WRITE_BATCH];
    PVOID diskAddress;
    ULONG index;
    HANDLE events[2];
    PPFN victims[MAXIMUM_WRITE_BATCH];
    PTHREAD_INFO threadInfo;
    ULONG numVictims;
    ULONG64 slotsAvailable[MAXIMUM_WRITE_BATCH];
    ULONG64 diskPagesFound;
    ULONG64 batchSize;

    batchSize = MAXIMUM_WRITE_BATCH;
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
        LeaveCriticalSection(&vmState.modifiedListLock);

        // TODO: Make this more efficient because we just linear walk through the disk
        // Look for a disk_page that is available
        // We will start from the last used disk index
        // // Flag to see if we got a disk index
        BOOL disk_slot_found = FALSE;
        diskPagesFound = 0;
        for(ULONG64 i = 0; i < NUMBER_OF_DISK_PAGES; i++) {
            // Here we check to see if we found a good index!
            if (vmState.disk_pages[vmState.disk_page_index] == 0) {
                slotsAvailable[diskPagesFound] = vmState.disk_page_index;
                diskPagesFound++;
                if (diskPagesFound == batchSize) {
                    disk_slot_found = TRUE;
                    break;
                }
            }
            // Otherwise, we check the next one
            vmState.disk_page_index++;
            // Check if we are at the end of the array and wrap around
            if (vmState.disk_page_index == NUMBER_OF_DISK_PAGES + 1) {
                vmState.disk_page_index = 1;
            }
        }
        if (diskPagesFound != 0) {
            disk_slot_found = TRUE;
        }
        batchSize = diskPagesFound;
        numVictims = 0;

        EnterCriticalSection(&vmState.modifiedListLock);
        for (int i = 0; i < batchSize; i++) {
            victims[i] = pop_page(&vmState.modifiedList);
            // Update page status to midwrite
            if (victims[i] == NULL) {
                break;
            }
            victims[i]->status = PFN_MIDWRITE;
            frameNumbers[i] = getFrameNumber(victims[i]);
            numVictims++;
        }
        LeaveCriticalSection(&vmState.modifiedListLock);

        // For a case where numVictims is not equal to/less than batchsize/diskPagesFound
        if(numVictims < batchSize){
            batchSize = numVictims;
        }
        // Means that there is no disk page available & we didn't pull any victims
        if (disk_slot_found == FALSE) {
            MapUserPhysicalPages(threadInfo->transferVA, numVictims, NULL);
            EnterCriticalSection(&vmState.modifiedListLock);
            for (int i = 0; i < numVictims; i++) {
                victims[i]->status = PFN_MODIFIED;
                add_entry(&vmState.modifiedList, victims[i]);
            }
            LeaveCriticalSection(&vmState.modifiedListLock);
        }
        else {
            // Temporarily map physical page to the transfer_va
            if (MapUserPhysicalPages(threadInfo->transferVA, numVictims, frameNumbers) == FALSE) {
                printf("write_to_disk : transfer_va is not mapped\n");
                DebugBreak();
            }
            for (int i = 0; i < numVictims; i++) {
                if (victims[i]->status == PFN_MIDWRITE) {
                    diskAddress = (PVOID)((ULONG64) vmState.disk + slotsAvailable[i] * PAGE_SIZE);
                    // Copy contents from transfer va to diskAddress
                    memcpy(diskAddress, (PVOID)((ULONG_PTR)threadInfo->transferVA + (i * PAGE_SIZE)), PAGE_SIZE);
                    // Make disk page unavailable
                    vmState.disk_pages[slotsAvailable[i]] = 1;
                    victims[i]->status = PFN_STANDBY;
                    victims[i]->diskIndex = vmState.disk_pages[slotsAvailable[i]];
                    EnterCriticalSection(&vmState.standbyListLock);
                    add_entry(&vmState.standbyList, victims[i]);
                    LeaveCriticalSection(&vmState.standbyListLock);
                }
                else {
                    // Should not be possible to NOT be mid-write because no soft faulting just yet
                    DebugBreak();
                }
            }
            // Unmap transfer VA
            if (MapUserPhysicalPages(threadInfo->transferVA, numVictims, NULL) == FALSE) {
                DebugBreak();
                printf("write_to_disk : transfer_va could not be unmapped");
            }
            SetEvent(vmState.finishWriter);
        }
    }
}
//
