//
// Created by diego.villegas on 7/10/2025.
#include "../include/pagefaulthandler.h"

#include "util.h"

void makePTEValid(PPTE pte, PPFN freePage, PULONG_PTR fault_va, ULONG64 frameNumber){
    PLIST_ENTRY head;

    // save the pte the pfn corresponds to
    freePage->pte = pte;
    // Update PTE to reflect what I did (later when we have to look for victims), store 40 bit frame number & set valid to 1
    // TODO: freePage->pte->validFormat.frameNumber = frameNumber;
    freePage->pte->validFormat.valid = 1;
    // Set our pfn status as active because we are connecting a va to a physical page
    freePage->status = PFN_ACTIVE;
    // get Head of active list and add our "free page" (which is now active) into our active list
    EnterCriticalSection(&vmState.activeListLock);
    head = &vmState.activeList;
    add_entry(head, freePage);
    LeaveCriticalSection(&vmState.activeListLock);
    // Map arbitrary_va to frame number
    if (MapUserPhysicalPages (fault_va, 1, &frameNumber) == FALSE) {
        DebugBreak();
        printf ("full_virtual_memory_test : could not map VA %p to page %llX\n", fault_va, frameNumber);
    }
}
void pageFaultHandler(PULONG_PTR fault_va, PTHREAD_INFO threadInfo) {
    //
    // Connect the virtual address now - if that succeeds then
    // we'll be able to access it from now on.
    //
    PPFN freePage;
    ULONG64 frameNumber;
    freePage = getFreePage();
    // We need the pte because the pte contains the information we need to (re)construct the va to the right contents
    PPTE pte = va_to_pte(fault_va);
    frameNumber = getFrameNumber(freePage);
    // Check if we saved the contents in disk because we trimmed the page earlier, so we need to retrieve
    // the contents into the new page we just got
    if (pte->DiskFormat.diskIndex != 0) {
        read_disk(pte->DiskFormat.diskIndex, frameNumber, threadInfo);
    }
    else {
        // VA that hasn't been connected to a physical page before case
        // Erase the old contents of the physical page because I don't want the new va to see old va's contents, we want a blank page
        zeroPage(frameNumber, threadInfo);
    }
    makePTEValid(pte, freePage, fault_va, frameNumber);
}
//
