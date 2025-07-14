//
// Created by diego.villegas on 7/9/2025.
#include "../include/trimmer.h"
#include "../include/pfn.h"
#include "../include/writer.h"
#include "../include/lists.h"
#include "../include/initializer.h"
void trim_page() {
    // Get the "oldest page" off the active list & remove it
    PPFN victim = find_victim(&activeList);
    if (victim == NULL) {
        printf("trim_page : victim from find_victim() is empty");
    }
    PPTE pte = victim->pte;
    if (pte == NULL) {
        printf("trim_page : victim pte is empty");
    }
    PULONG_PTR virtual_address = pte_to_va(pte);
    // Unmap pte
    if (MapUserPhysicalPages(virtual_address, 1, NULL) == FALSE) {
        DebugBreak();
        printf("trim_page : VA could not be unmapped");
    }
    // write to disc
    if (write_to_disk(victim->frameNumber)){
        // update pte in pfn
        victim->pte->invalidFormat.mustBeZero = 0;
        victim->pte->invalidFormat.diskIndex = disk_page_index;
    }
    else {
        victim->pte->entireFormat = 0;
    }
    // Set status as free because we are adding the victim to our freelist
    victim->status = PFN_FREE;
    add_entry(&freeList, victim);
}
//
