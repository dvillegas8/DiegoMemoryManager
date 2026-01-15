//
// Created by diego.villegas on 7/9/2025.
#include "../include/lists.h"

#include "util.h"

// Method which adds a new entry into our free list
void add_entry(PLIST_ENTRY head, PFN* newpfn) {
    PLIST_ENTRY previousNewestEntry = head->Blink;
    // Makes new entry/pfn Blink point towards the entry before it
    newpfn->entry.Blink = previousNewestEntry;
    // Make the "entry before" Flink point towards our new entry/pfn
    previousNewestEntry->Flink = &newpfn->entry;
    // Make the new entry/pfn Flink point towards the head
    newpfn->entry.Flink = head;
    // Make the head Blink point towards our new entry/pfn
    head->Blink = &newpfn->entry;
}

// Get a free page from the free list
PPFN pop_page(PLIST_ENTRY head) {
    // Check for empty list
    if (head->Flink == head) {
        return NULL;
    }
    // Get the page/PFN we want to remove
    PPFN freePage = (PPFN)head->Flink;
    // Get the PFN after our freepage
    PPFN nextPage = (PPFN)freePage->entry.Flink;
    // Make head Flink point towards nextpage
    head->Flink = &nextPage->entry;
    // Make nextpage blink point towards head
    nextPage->entry.Blink = head;
    ASSERT(isValidFrame(freePage));
    // Make page we removed point towards itself as it doesn't belong to any list at the moment
    freePage->entry.Flink = &freePage->entry;
    freePage->entry.Blink = &freePage->entry;
    return freePage;
}

// Get a page from the active list
PPFN find_victim(PLIST_ENTRY head) {
    if (head->Flink == head) {
        printf("find_victim : empty list");
        return NULL;
    }
    // Grab the last page in the list
    PPFN victim = (PPFN)head->Blink;
    // Grab the 2nd to last page
    PPFN pageBefore = (PPFN)victim->entry.Blink;
    // Make 2nd to last page point towards head
    pageBefore->entry.Flink = head;
    // Make head blink point towards 2nd to last page
    head->Blink = &pageBefore->entry;
    return victim;
}
PPFN getFreePage() {
    PLIST_ENTRY head;
    PPFN freePage;

    freePage = NULL;
    while(freePage == NULL){
        EnterCriticalSection(&vmState.freeListLock);
        head = &vmState.freeList;
        // Check if Free list is empty
        if(!IsListEmpty(head)) {
            // Get a free page from the free list
            freePage = pop_page(head);
        }
        LeaveCriticalSection(&vmState.freeListLock);
        if (freePage == NULL) {
            EnterCriticalSection(&vmState.pageTableLock);
            EnterCriticalSection(&vmState.standbyListLock);
            head = &vmState.standbyList;
            // Check if standby list is empty
            if(!IsListEmpty(head)) {
                // Get a free page from the standby list
                freePage = pop_page(head);
                EnterCriticalSection(&vmState.pageLocks[freePage->lockIndex]);
                vmState.pageLocksStatus[freePage->lockIndex] = 1;
                ASSERT(freePage->diskIndex < NUMBER_OF_DISK_PAGES);
                // Turn old PTE into disk format
                PPTE old_pte = freePage->pte;
                ASSERT(old_pte);
                ASSERT(old_pte->DiskFormat.invalid == 0);
                // Save the contents (which is on disk) of the page into the original PTE because its page is being
                // repurposed
                old_pte->DiskFormat.diskIndex = freePage->diskIndex;
                // Clear diskIndex in page that was repurposed for a hard fault
                freePage->diskIndex = 0;
                old_pte->DiskFormat.status = PTE_ON_DISK;
                vmState.pageLocksStatus[freePage->lockIndex] = 0;
                LeaveCriticalSection(&vmState.pageLocks[freePage->lockIndex]);
            }
            LeaveCriticalSection(&vmState.standbyListLock);
            LeaveCriticalSection(&vmState.pageTableLock);
        }
        if (freePage == NULL) {
            SetEvent(vmState.startTrimmer);
            WaitForSingleObject(vmState.finishWriter, INFINITE);
        }
    }
    return freePage;
}
// Helps us remove a page from anywhere in a list
void removePage(PLIST_ENTRY pageRemoved) {
    // Get the page after
    PLIST_ENTRY after = pageRemoved->Flink;
    // Get the page before
    PLIST_ENTRY previous = pageRemoved->Blink;
    // Connect
    after->Blink = previous;
    previous->Flink = after;
    // Page doesn't belong to any list, make it point towards itself
    pageRemoved->Flink = pageRemoved;
    pageRemoved->Blink = pageRemoved;
}

//
