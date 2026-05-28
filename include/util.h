//
// Created by diego.villegas on 7/9/2025.
//

#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <stddef.h>
#include "initializer.h"



boolean isValidFrame(PPFN pfn);
void checkVa(PULONG64 va);
void zeroPage(ULONG64 frameNumber, PTHREAD_INFO threadInfo);
BOOL GetPrivilege(VOID);
void clearDiskSlot(ULONG64 diskIndex);
ULONG64 getPTERegionLock(PPTE pte);
void enterPTELock(PULONG_PTR virtual_address);
void leavePTELock(PULONG_PTR virtual_address);

#endif //UTIL_H
