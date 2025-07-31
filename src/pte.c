//
// Created by diego.villegas on 7/9/2025.
#include "../include/pte.h"
#include "../include/initializer.h"

PPTE va_to_pte(PULONG_PTR virtual_address) {
    ULONG64 index = ((ULONG64) virtual_address - (ULONG64) vmState.vaBase) / PAGE_SIZE;
    PPTE pte = vmState.pageTable + index;
    return pte;
}

PULONG_PTR pte_to_va(PPTE pte) {
    ULONG64 index = pte - vmState.pageTable;
    PULONG_PTR va = (PULONG_PTR)((index * PAGE_SIZE) + (ULONG64) vmState.vaBase);
    return va;
}
