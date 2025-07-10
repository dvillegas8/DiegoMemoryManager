//
// Created by diego.villegas on 7/9/2025.
#include "../include/util.h"
#include "../include/initializer.h"
void checkVa(PULONG64 va) {
    va = (PULONG64) ((ULONG64)va & ~(PAGE_SIZE - 1));
    for (int i = 0; i < PAGE_SIZE / 8; ++i) {
        if (!(*va == 0 || *va == (ULONG64) va)) {
            DebugBreak();
        }
        va += 1;
    }
}
void zeroPage(ULONG64 frameNumber) {
    if (MapUserPhysicalPages(transfer_va, 1, &frameNumber) == FALSE) {
        DebugBreak();
        printf("read_to_disk : transfer_va could not be mapped");
    }
    memset(transfer_va, 0, PAGE_SIZE);
    if (MapUserPhysicalPages(transfer_va, 1, NULL) == FALSE) {
        DebugBreak();
        printf("read_to_disk : transfer_va could not be unmapped");
    }
}
//
