//
// Created by diego.villegas on 7/9/2025.
// Look for a free disk index
#include "../include/writer.h"
#include "../include/initializer.h"
VOID writeToDisk(PVOID context) {
    ULONG64 frameNumber;
    ULONG64 counter;
    PVOID diskAddress;
    ULONG index;
    HANDLE events[2];
    PPFN victim;

    events[START_EVENT_INDEX] = startWriter;
    events[EXIT_EVENT_INDEX] = exitEvent;
    WaitForSingleObject(startEvent, INFINITE);
    while (TRUE){
        index = WaitForMultipleObjects (ARRAYSIZE(events), events, FALSE, INFINITE);
        if (index == EXIT_EVENT_INDEX) {
            printf("writer exit event\n");
            return;
        }
        victim = pop_page(&modifiedList);
        frameNumber = getFrameNumber(victim);
        // Temporarily map physical page to the transfer_va
        if (MapUserPhysicalPages(transfer_va, 1, &frameNumber) == FALSE) {
            DebugBreak();
            printf("write_to_disk : transfer_va is not mapped\n");
        }
        counter = 0;
        // TODO: Make this more efficient because we just linear walk through the disk
        // Look for a disk_page that is available
        // We will start from the last used disk index
        // // Flag to see if we got a disk index
        BOOL disk_slot_found = FALSE;
        for(ULONG64 i=0; i<NUMBER_OF_DISK_PAGES; i++) {

            // Here we check to see if we found a good index!
            if (disk_pages[disk_page_index] == 0) {
                disk_slot_found = TRUE;
                break;
            }

            // Otherwise, we check the next one
            disk_page_index++;

            // Check if we are at the end of the array and wrap around
            if (disk_page_index == NUMBER_OF_DISK_PAGES + 1) disk_page_index = 1;
        }
        // Means that there is no disk page available
        if (disk_slot_found == FALSE) {
            // TODO: NOTE if you remove this our disk gets full
            MapUserPhysicalPages(transfer_va, 1, NULL);
            victim->pte->entireFormat = 0;
            victim->status = PFN_STANDBY;
            add_entry(&standbyList, victim);
            SetEvent(finishWriter);
            printf("Disk is full");
            DebugBreak();
        }
        else {
            diskAddress = (PVOID)((ULONG64) disk + disk_page_index * PAGE_SIZE);
            // Copy contents from transfer va to diskAddress
            memcpy(diskAddress, transfer_va, PAGE_SIZE);
            // Make disk page unavailable
            disk_pages[disk_page_index] = 1;
            // Unmap transfer VA
            if (MapUserPhysicalPages(transfer_va, 1, NULL) == FALSE) {
                DebugBreak();
                printf("write_to_disk : transfer_va could not be unmapped");
            }
            victim->status = PFN_STANDBY;
            victim->pte->DiskFormat.mustBeZero = 0;
            victim->pte->DiskFormat.diskIndex = disk_page_index;
            add_entry(&standbyList, victim);
            // update pte in pfn
            SetEvent(finishWriter);
        }
    }
}
//
