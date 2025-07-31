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
#include "../include/trimmer.h"
#include "../macros.h"
#include "../include/initializer.h"
// TODO: intermediate list to potentially have when we have multiple writer threads,
// TODO: we put all of the pages that finished writing into disk into this list and eventually goes on standby

// Lists functions
void add_entry(PLIST_ENTRY head, PFN* newpfn);
PPFN pop_page(PLIST_ENTRY head);
PPFN find_victim(PLIST_ENTRY head);
PPFN getFreePage();
void removePage(PLIST_ENTRY head);

#endif //LISTS_H
