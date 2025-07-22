//
// Created by diego.villegas on 7/9/2025.
#include "../include/lists.h"

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
        printf("pop_page : empty list");
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

    head = &freeList;
    // Check if Free list is empty
    if(IsListEmpty(head)){
        trim_page();
        // TODO: since we trimmed, our trimmed page is in our stanbylist
        head = &standbyList;
    }
    // Get a free page from the free list
    freePage = pop_page(head);
    if (freePage == NULL) {
        printf("full_virtual_memory_test: freePage is null");
    }
    return freePage;
}
// TODO: Create a function where we remove a page of a list without returning it
// This is just for modfied since we already have access to the victim page
void removePage(PLIST_ENTRY head) {
    if (head->Flink == head) {
        printf("removePage : empty list");
        return;
    }
    // Get the page/PFN we want to remove
    PPFN pageRemoved = (PPFN)head->Flink;
    // Get the PFN after our freepage
    // TODO: will this prove an issue if we only have 1 PFN in the list? I am asking this because if we have 1 PFN in
    // TODO: The list, the line below will cast the head of the list as a
    PPFN nextPage = (PPFN)pageRemoved->entry.Flink;
    if (nextPage == NULL) {
        DebugBreak();
    }
    // Make head Flink point towards nextpage
    head->Flink = &nextPage->entry;
    // Make nextpage blink point towards head
    nextPage->entry.Blink = head;
}

//
