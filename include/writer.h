//
// Created by diego.villegas on 7/9/2025.
//

#ifndef WRITER_H
#define WRITER_H
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "../include/initializer.h"
#include <stdbool.h>
#include <stddef.h>

#define MAXIMUM_WRITE_BATCH     10
VOID writeToDisk(PVOID context);

#endif //WRITER_H
