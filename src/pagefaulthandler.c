//
// Created by diego.villegas on 7/10/2025.
#include "../include/pagefaulthandler.h"


#include "util.h"

void makePTEValid(PPTE pte, PPFN freePage, PULONG_PTR fault_va, ULONG64 frameNumber){
    PLIST_ENTRY head;
    ASSERT(freePage->entry.Flink == &freePage->entry);
    // save the pte the pfn corresponds to
    freePage->pte = pte;
    // Update PTE to reflect what I did (later when we have to look for victims), store 40 bit frame number & set valid to 1
    EnterCriticalSection(&vmState.pageTableLock);
    pte->validFormat.valid = 1;
    pte->validFormat.status = 0;
    pte->validFormat.frameNumber = frameNumber;
    LeaveCriticalSection(&vmState.pageTableLock);
    // get Head of active list and add our "free page" (which is now active) into our active list
    EnterCriticalSection(&vmState.activeListLock);
    EnterCriticalSection(&vmState.pageLocks[freePage->lockIndex]);
    vmState.pageLocksStatus[freePage->lockIndex] = 1;
    vmState.pageLocksStatusThreads[0] = 1;
    // Set our pfn status as active because we are connecting a va to a physical page
    ASSERT(freePage->entry.Flink == &freePage->entry);
    freePage->status = PFN_ACTIVE;
    head = &vmState.activeList;
    add_entry(head, freePage);
    vmState.pageLocksStatusThreads[0] = 0;
    vmState.pageLocksStatus[freePage->lockIndex] = 0;
    LeaveCriticalSection(&vmState.pageLocks[freePage->lockIndex]);
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
    PPTE pte;

    // We need the pte because the pte contains the information we need to (re)construct the va to the right contents
    pte = va_to_pte(fault_va);
    // Check if pte is in transition format in order to do soft faulting, reclaiming back its page
    if (IS_PTE_TRANSITION(pte)) {
        // Soft fault/ rescue
        frameNumber = pte->transitionFormat.frameNumber;
        freePage = vmState.PFN_base + frameNumber;
        // Check where the rescued page is so that we can grab the lock and remove it from that list
        if(freePage->status == PFN_MODIFIED){
            EnterCriticalSection(&vmState.modifiedListLock);
            EnterCriticalSection(&vmState.pageLocks[freePage->lockIndex]);
            vmState.pageLocksStatus[freePage->lockIndex] = 1;
            // Doublecheck the status of the page for a potential race condition
            if (freePage->status == PFN_MODIFIED) {
                removePage(&freePage->entry);
                freePage->status = PFN_MIDRESCUE;
            }
            vmState.pageLocksStatus[freePage->lockIndex] = 0;
            LeaveCriticalSection(&vmState.pageLocks[freePage->lockIndex]);
            LeaveCriticalSection(&vmState.modifiedListLock);
        }
        if (freePage->status == PFN_MIDWRITE) {
            EnterCriticalSection(&vmState.pageLocks[freePage->lockIndex]);
            if (freePage->status == PFN_MIDWRITE) {
                freePage->status = PFN_MIDRESCUE;
            }
            LeaveCriticalSection(&vmState.pageLocks[freePage->lockIndex]);
        }
        if (freePage->status == PFN_STANDBY){
            EnterCriticalSection(&vmState.standbyListLock);
            EnterCriticalSection(&vmState.pageLocks[freePage->lockIndex]);
            vmState.pageLocksStatus[freePage->lockIndex] = 1;
            // Doublecheck the status of the page for a potential race condition
            if (freePage->status == PFN_STANDBY) {
                removePage(&freePage->entry);
                freePage->status = PFN_MIDRESCUE;
            }
            vmState.pageLocksStatus[freePage->lockIndex] = 0;
            LeaveCriticalSection(&vmState.pageLocks[freePage->lockIndex]);
            LeaveCriticalSection(&vmState.standbyListLock);
            clearDiskSlot(freePage->diskIndex);
        }
        // Keep in mind of a case where we grab a page mid write, however we don't need to do anything since
        // the page doesn't belong to a list for now
    }
    else {
        freePage = getFreePage();
        frameNumber = getFrameNumber(freePage);
        // Check if we saved the contents in disk because we trimmed the page earlier, so we need to retrieve
        // the contents into the new page we just got
        if (pte->DiskFormat.diskIndex != 0) {
            read_disk(pte->DiskFormat.diskIndex, frameNumber, threadInfo);
        }
        else {
            // VA that hasn't been connected to a physical page before case
            // Erase the old contents of the physical page because I don't want the new va to see old va's contents,
            // we want a blank page
            zeroPage(frameNumber, threadInfo);
        }
    }
    // acquire page table lock
    // check this pte -- he MIGHT be valid
    // if he IS valid (someone else reolved the fault) then put freePage on the free list

    // otherwise -- he is still faulting:
    ASSERT(freePage->entry.Flink == &freePage->entry);
    makePTEValid(pte, freePage, fault_va, frameNumber);
    // release the page table lock

    vmState.NumberOfFaults += 1;
}
//
