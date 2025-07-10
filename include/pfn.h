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

// PFN struct
typedef struct {
    // Help us create our doubly linked list
    LIST_ENTRY entry;
    // Pointer of the PTE this pfn belongs to
    PPTE pte;
    // Index that maps to a place in physical memory
    ULONG64 frameNumber;

} PFN, *PPFN;

// PFN functions


#endif //PFN_H
