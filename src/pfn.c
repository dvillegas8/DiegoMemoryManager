//
#include "../include/pfn.h"
// Created by diego.villegas on 7/9/2025.
ULONG64 getFrameNumber(PPFN pfn) {
    ULONG64 frameNumber = pfn - PFN_array;
    return frameNumber;
}
//
