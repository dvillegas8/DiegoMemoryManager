//
// Created by diego.villegas on 7/9/2025.
//

#ifndef LISTS_H
#define LISTS_H
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <stddef.h>
#include "../include/pfn.h"
// Lists variables
// Our doubly linked list containing all of our free pages
LIST_ENTRY freeList;
// Our doubly linked list containing all of our active pages
LIST_ENTRY activeList;
LIST_ENTRY modifiedList;

// Lists functions
void add_entry(PLIST_ENTRY head, PFN* newpfn);
PPFN pop_page(PLIST_ENTRY head);
PPFN find_victim(PLIST_ENTRY head);

#endif //LISTS_H
