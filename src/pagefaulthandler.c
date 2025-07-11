//
// Created by diego.villegas on 7/10/2025.
#include "../include/pagefaulthandler.h"
void makePTEValid(PPTE pte, PPFN freePage, PULONG_PTR fault_va){
    PLIST_ENTRY head;
    ULONG64 frameNumber;

    frameNumber = freePage->frameNumber;
    // save the pte the pfn corresponds to
    freePage->pte = pte;
    // Update PTE to reflect what I did (later when we have to look for victims), store 40 bit frame number & set valid to 1
    freePage->pte->validFormat.frameNumber = frameNumber;
    freePage->pte->validFormat.valid = 1;
    // get Head of active list and add our "free page" (which is now active) into our active list
    head = &activeList;
    add_entry(head, freePage);
    // Map arbitrary_va to frame number
    if (MapUserPhysicalPages (fault_va, 1, &frameNumber) == FALSE) {
        DebugBreak();
        printf ("full_virtual_memory_test : could not map VA %p to page %llX\n", fault_va, frameNumber);

        return;
    }
}
void pageFaultHandler(PULONG_PTR fault_va) {
    //
    // Connect the virtual address now - if that succeeds then
    // we'll be able to access it from now on.
    //
    PPFN freePage = getFreePage();
    PLIST_ENTRY head;
    // We need the pte because the pte contains the information we need to (re)construct the va to the right contents
    PPTE pte = va_to_pte(fault_va);
    ULONG64 frameNumber = freePage->frameNumber;

    // Check if we saved the contents in disk because we trimmed the page earlier, so we need to retrieve
    // the contents into the new page we just got
    if (pte->invalidFormat.diskIndex != 0) {
        read_disk(pte->invalidFormat.diskIndex, frameNumber);
    }
    else {
        // VA that hasn't been connected to a physical page before case
        // Erase the old contents of the physical page because I don't want the new va to see old va's contents, we want a blank page
        zeroPage(frameNumber);
    }
    makePTEValid(pte, freePage, fault_va);
}
//
