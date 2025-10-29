//
#include "../include/pfn.h"

#include "initializer.h"
// Created by diego.villegas on 7/9/2025.
ULONG64 getFrameNumber(PPFN pfn) {
    ULONG64 frameNumber = pfn - vmState.PFN_base;
    return frameNumber;
}
//
