//
// Created by diego.villegas on 7/9/2025.
#include "../include/trimmer.h"
#include "../include/pfn.h"
#include "../include/writer.h"
#include "../include/lists.h"
#include "../include/initializer.h"
VOID trimPage(PVOID context) {
    ULONG index;
    HANDLE events[2];

    events[START_EVENT_INDEX] = startTrimmer;
    events[EXIT_EVENT_INDEX] = exitEvent;
    WaitForSingleObject(startEvent, INFINITE);
    while (TRUE) {
        index = WaitForMultipleObjects (ARRAYSIZE(events), events, FALSE,
                                           INFINITE);
        if (index == EXIT_EVENT_INDEX) {
            printf("trimmer exit event\n");
            return;
        }

        PPFN victim;
        PPTE pte;
        PULONG_PTR virtual_address;
        ULONG64 frameNumber;

        // TODO: find victim doesnt return the oldest page, instead use pop page (JUST DOUBLE CHECK)
        // Get the "oldest page" off the active list & remove it
        victim = find_victim(&activeList);
        // Make victim page modified because we trimmed it and about to add it to disk
        victim->status = PFN_MODIFIED;
        // Add victim to modified list
        add_entry(&modifiedList, victim);
        if (victim == NULL) {
            printf("trim_page : victim from find_victim() is empty");
        }
        pte = victim->pte;
        if (pte == NULL) {
            printf("trim_page : victim pte is empty");
        }
        virtual_address = pte_to_va(pte);
        // Unmap pte
        if (MapUserPhysicalPages(virtual_address, 1, NULL) == FALSE) {
            DebugBreak();
            printf("trim_page : VA could not be unmapped");
        }
        frameNumber = getFrameNumber(victim);
        // write to disc
        SetEvent(startWriter);
        /*
        // TODO: Stuff below isn't necessary for me anymore
        if (write_to_disk(frameNumber)){
            removePage(&modifiedList);
            victim->status = PFN_STANDBY;
            add_entry(&standbyList, victim);
            // update pte in pfn
            victim->pte->invalidFormat.mustBeZero = 0;
            victim->pte->invalidFormat.diskIndex = disk_page_index;
        }
        else {
            victim->pte->entireFormat = 0;
            removePage(&modifiedList);
            victim->status = PFN_STANDBY;
            add_entry(&standbyList, victim);
        }
        */
        // Set status as free because we are adding the victim to our freelist
        // victim->status = PFN_FREE;
        //add_entry(&freeList, victim);
    }
    return;
}
//
