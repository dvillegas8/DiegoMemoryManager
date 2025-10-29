//
// Created by diego.villegas on 7/9/2025.
#include "../include/trimmer.h"
#include "../include/pfn.h"
#include "../include/writer.h"
#include "../include/lists.h"
#include "../include/initializer.h"

VOID trimPage(PVOID context) {
    PTHREAD_INFO threadInfo;
    ULONG threadIndex;
    HANDLE events[2];
    PPFN victim;
    PPTE pte;
    PULONG_PTR virtual_address;
    PULONG_PTR transferVA;
    PPFN victims[MAXIMUM_TRIM_BATCH];
    ULONG numVictims;
    PPTE ptes[MAXIMUM_TRIM_BATCH];
    PULONG_PTR virtualAddresses[MAXIMUM_TRIM_BATCH];
    ULONG64 frameNumbers[MAXIMUM_TRIM_BATCH];

    // Get all thread information
    threadInfo = (PTHREAD_INFO)context;
    threadIndex = threadInfo->ThreadNumber;
    events[START_EVENT_INDEX] = vmState.startTrimmer;
    events[EXIT_EVENT_INDEX] = vmState.exitEvent;
    WaitForSingleObject(vmState.startEvent, INFINITE);
    while (TRUE) {
        threadIndex = WaitForMultipleObjects (ARRAYSIZE(events), events, FALSE,
                                           INFINITE);
        if (threadIndex == EXIT_EVENT_INDEX) {
            printf("trimmer exit event\n");
            return;
        }
        numVictims = 0;
        // Get the "oldest page" off the active list & remove it
        EnterCriticalSection(&vmState.activeListLock);
        for (int i = 0; i < MAXIMUM_TRIM_BATCH; i++) {
            victims[i] = pop_page(&vmState.activeList);
            if (victims[i] == NULL) {
                break;
            }
            ptes[i] = victims[i]->pte;
            if (ptes[i] == NULL) {
                printf("trim_page : victim pte is empty");
                DebugBreak();
            }
            virtualAddresses[i] = pte_to_va(ptes[i]);
            frameNumbers[i] = getFrameNumber(victims[i]);
            numVictims++;
        }
        LeaveCriticalSection(&vmState.activeListLock);
        if (numVictims == 0) {
            continue;
        }
        // Unmap ptes
        if (MapUserPhysicalPagesScatter(virtualAddresses, numVictims, NULL) == FALSE) {
            DebugBreak();
            printf("trim_page : VA could not be unmapped");
        }

        // Make victim page modified because we trimmed it and about to add it to disk
        EnterCriticalSection(&vmState.modifiedListLock);
        for (int i = 0; i < numVictims; i++) {
            victims[i]->status = PFN_MODIFIED;
            EnterCriticalSection(&vmState.pageTableLock);
            ptes[i]->transitionFormat.invalid = 0;
            ptes[i]->transitionFormat.status = 1;
            ptes[i]->transitionFormat.frameNumber = frameNumbers[i];
            LeaveCriticalSection(&vmState.pageTableLock);
            // Add victim to modified list
            add_entry(&vmState.modifiedList, victims[i]);
        }
        LeaveCriticalSection(&vmState.modifiedListLock);

        // Begin writing to disk
        SetEvent(vmState.startWriter);
    }
    return;
}
//
