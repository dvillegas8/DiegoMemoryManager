//
// Created by diego.villegas on 7/10/2025.
//

#ifndef PAGEFAULTHANDLER_H
#define PAGEFAULTHANDLER_H
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <stddef.h>
#include "../include/lists.h"
#include "../include/reader.h"
#include "initializer.h"
// pagefaulthandler functions
void makePTEValid();
void pageFaultHandler(PULONG_PTR fault_va, PTHREAD_INFO threadInfo);
#endif //PAGEFAULTHANDLER_H
