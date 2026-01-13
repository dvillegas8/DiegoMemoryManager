//
// Created by diego.villegas on 7/9/2025.
//

#ifndef PFN_H
#define PFN_H

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <stddef.h>
#include "../include/pte.h"


// PFN status
#define PFN_FREE 0x0
#define PFN_ACTIVE 0x1
#define PFN_MODIFIED 0x2
#define PFN_STANDBY 0x3
#define PFN_MIDWRITE 0x4




// PFN struct
typedef struct {
    // Help us create our doubly linked list
    LIST_ENTRY entry;
    // Pointer of the PTE this pfn belongs to
    PPTE pte;
    // Indicates the status of the PFN/which list it is currently on
    ULONG64 status: 3;
    // Helps us locate the contents of the physical page that are saved onto disk because the page
    // For when we soft fault
    ULONG64 diskIndex: 40;
    // Index to keep track this page's lock
    ULONG64 lockIndex;
} PFN, *PPFN;

// PFN functions
ULONG64 getFrameNumber(PPFN pfn);


#endif //PFN_H
