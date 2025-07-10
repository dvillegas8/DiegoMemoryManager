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

void checkVa(PULONG64 va);
void zeroPage(ULONG64 frameNumber);

#endif //UTIL_H
