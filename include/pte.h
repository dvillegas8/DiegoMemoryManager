//
// Created by diego.villegas on 7/9/2025.
//

#ifndef PTE_H
#define PTE_H
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <stddef.h>

// PTE structs

// Pte = Page Table Entry
// Struct has 64 bits available
// Note, the number all the way to the right is how many bits we allocate to the field
typedef struct {
    // Bit that tells us that pte is active, meaning pte maps to a valid physical page
    ULONG64 valid: 1;
    // Page frame number, maps to an actual physical page, 40 bits because we can shave off 12 bits from the
    // physical address. If we want the physical address back, we multiply by 4k
    ULONG64 frameNumber: 40;
} validPte;

typedef struct {
    // Bit that tell us that this pte is not active, meaning pte is not mapped into physical memory
    ULONG64 mustBeZero: 1;
    // Helps us locate the contents of the physical page that are saved onto disk because the page
    // might have become a victim of our trimming (creating a free page)
    ULONG64 diskIndex: 40;
    // How many bits we have left
    ULONG64 reserve: 23;
} invalidPte;

typedef struct {
    // Since we make so many pte, we want to union the fields to save space (Union allows multiple variables to
    // share the same memory space) = overlay
    union {
        validPte validFormat;
        invalidPte invalidFormat;
        ULONG64 entireFormat;
    };
} PTE, *PPTE;

// PTE functions
PPTE va_to_pte(PULONG_PTR virtual_address);
PULONG_PTR pte_to_va(PPTE pte);



#endif //PTE_H
