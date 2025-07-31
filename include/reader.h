//
// Created by diego.villegas on 7/9/2025.
//

#ifndef READER_H
#define READER_H
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <stddef.h>
#include "initializer.h"


void read_disk(ULONG64 disk_index, ULONG64 frameNumber, PTHREAD_INFO threadInfo);

#endif //READER_H
