//
// Created by diego.villegas on 7/30/2025.
//

#ifndef DTHREADS_H
#define DTHREADS_H
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "../include/pfn.h"
#include "../include/util.h"
#include "../include/main.h"
// Thread information
#define DEFAULT_SECURITY        ((LPSECURITY_ATTRIBUTES) NULL)
#define DEFAULT_STACK_SIZE      0
#define DEFAULT_CREATION_FLAGS  0
#define AUTO_RESET              FALSE

#define NUM_OF_USER_THREADS     1
#define NUM_OF_TRIMMER_THREADS  1
#define NUM_OF_WRITER_THREADS   1

#define MAX_NUMBER_OF_THREADS   (NUM_OF_USER_THREADS + NUM_OF_TRIMMER_THREADS + NUM_OF_WRITER_THREADS)



#endif //DTHREADS_H
