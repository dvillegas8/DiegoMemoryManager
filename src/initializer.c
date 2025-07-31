//
// Created by diego.villegas on 7/9/2025.
#include "../include/initializer.h"
#include "dthreads.h"
#include "../macros.h"
#include "../include/lists.h"
VMState vmState;
// Initialize both active and free list and add pfns to free list
void initialize_lists (PULONG_PTR physical_page_numbers, PPFN pfnarray, ULONG_PTR physical_page_count) {
    // Create the head of our active list
    InitializeListHead(&vmState.activeList);
    // Create the Head of our free list
    InitializeListHead(&vmState.freeList);
    // Create the head of our modified list
    InitializeListHead(&vmState.modifiedList);
    // Create the head of our standby list
    InitializeListHead(&vmState.standbyList);
}
void initializeSparseArray(PULONG_PTR physical_page_numbers) {
    PPFN pfn;
    for (int i = 0; i < NUMBER_OF_PHYSICAL_PAGES; i++){
        // Commit physical memory to this spot in the PFN array since we will be mapping the frame number to this
        // specific pfn
        LPVOID result = VirtualAlloc((vmState.PFN_array + physical_page_numbers[i]), sizeof(PFN),
            MEM_COMMIT, PAGE_READWRITE);
        if (result == NULL) {
            printf("initialize_sparse_array: VirtualAlloc failed\n");
        }
        pfn = vmState.PFN_array + physical_page_numbers[i];
        pfn->status = PFN_FREE;
        // TODO: check in about zeroing out this page
        add_entry(&vmState.freeList, pfn);
    }
}

void initialize_disk_space() {
    // VA space - PFN space ex: 10 virtual pages - 3 PFN pages = 7 disk pages, having 10 would be a waste
    // We want 7 in this example so that we can have enough space to do swapping
    // We add PAGE_SIZE to help us swap a disk page and physical page when all disk pages are full
    vmState.disk = malloc(DISK_SIZE_IN_BYTES);
    if (vmState.disk == NULL) {
        printf("initialize_disk_space : disk_space malloc failed");
    }
    memset(vmState.disk, 0, DISK_SIZE_IN_BYTES);
    vmState.disk_pages = malloc(DISK_SIZE_IN_BYTES / PAGE_SIZE);
    if (vmState.disk_pages == NULL) {
        printf("initialize_disk_space : disk_page malloc failed");
    }
    // 0 means that the disk page is available, 1 means the disk page is in use
    memset(vmState.disk_pages, 0, DISK_SIZE_IN_BYTES / PAGE_SIZE);
    // Skip index 0 so that when we page fault, we can correctly check diskIndex in PTE invalid format
    vmState.disk_page_index = 1;
}

void initializeEvents() {
    vmState.startEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
    vmState.startTrimmer = CreateEvent(NULL, FALSE, FALSE, NULL);
    vmState.finishTrimmer = CreateEvent(NULL, FALSE, FALSE, NULL);
    vmState.exitEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
    vmState.startWriter = CreateEvent (NULL, FALSE, FALSE, NULL);
    vmState.finishWriter = CreateEvent (NULL, FALSE, FALSE, NULL);
}
void initializeThreads() {
    vmState.userThreads = malloc(sizeof(THREAD_INFO) * NUM_OF_USER_THREADS);
    for (int i = 0; i < NUM_OF_USER_THREADS; i++) {
        vmState.userThreads[i].ThreadNumber = i;
        vmState.userThreads[i].transferVA = (PVOID) vmState.userTransferVAs[i];
        vmState.userThreads[i].ThreadHandle = CreateThread (DEFAULT_SECURITY,
                               DEFAULT_STACK_SIZE,
                               accessVirtualMemory,
                               &vmState.userThreads[i],
                               DEFAULT_CREATION_FLAGS,
                               &vmState.userThreads[i].ThreadId);
        if (vmState.userThreads[i].ThreadHandle == NULL) {
            DebugBreak();
            printf ("could not create user thread\n");
        }
    }
    vmState.trimmerThreads = malloc(sizeof(THREAD_INFO) * NUM_OF_TRIMMER_THREADS);
    for (int i = 0; i < NUM_OF_TRIMMER_THREADS; i++) {
        vmState.trimmerThreads[i].ThreadNumber = i;
        vmState.trimmerThreads[i].transferVA = (PVOID) vmState.trimmerTransferVAs[i];
        vmState.trimmerThreads[i].ThreadHandle = CreateThread (DEFAULT_SECURITY,
                               DEFAULT_STACK_SIZE,
                               (LPTHREAD_START_ROUTINE) trimPage,
                               &vmState.trimmerThreads[i],
                               DEFAULT_CREATION_FLAGS,
                               &vmState.trimmerThreads[i].ThreadId);
        if (vmState.trimmerThreads[i].ThreadHandle == NULL) {
            DebugBreak();
            printf ("could not create trimmer thread\n");
        }
    }
    vmState.writerThreads = malloc(sizeof(THREAD_INFO) * NUM_OF_WRITER_THREADS);
    for (int i = 0; i < NUM_OF_WRITER_THREADS; i++) {
        vmState.writerThreads[i].ThreadNumber = i;
        vmState.writerThreads[i].transferVA = (PVOID) vmState.writerTransferVAs[i];
        vmState.writerThreads[i].ThreadHandle = CreateThread (DEFAULT_SECURITY,
                               DEFAULT_STACK_SIZE,
                               (LPTHREAD_START_ROUTINE) writeToDisk,
                               &vmState.writerThreads[i],
                               DEFAULT_CREATION_FLAGS,
                               &vmState.writerThreads[i].ThreadId);
        if (vmState.writerThreads[i].ThreadHandle == NULL) {
            DebugBreak();
            printf ("could not create trimmer thread\n");
        }
    }
}
void initializeLocks() {
    InitializeCriticalSection(&vmState.bigLock);
    InitializeCriticalSection(&vmState.freeListLock);
    InitializeCriticalSection(&vmState.activeListLock);
    InitializeCriticalSection(&vmState.readingLock);
    InitializeCriticalSection(&vmState.zeroingPageLock);
    InitializeCriticalSection(&vmState.modifiedListLock);
    InitializeCriticalSection(&vmState.standByListLock);
}
void initializeVirtualMemory() {
    BOOL allocated;
    BOOL privilege;
    BOOL obtained_pages;
    ULONG_PTR physical_page_count;
    HANDLE physical_page_handle;
    ULONG_PTR virtual_address_size;

    //
    // Allocate the physical pages that we will be managing.
    //
    // First acquire privilege to do this since physical page control
    // is typically something the operating system reserves the sole
    // right to do.
    //

    privilege = GetPrivilege();

    if (privilege == FALSE) {
        printf ("full_virtual_memory_test : could not get privilege\n");
        return;
    }

#if SUPPORT_MULTIPLE_VA_TO_SAME_PAGE

    physical_page_handle = CreateSharedMemorySection ();

    if (physical_page_handle == NULL) {
        printf ("CreateFileMapping2 failed, error %#x\n", GetLastError ());
        return;
    }

#else

    physical_page_handle = GetCurrentProcess ();

#endif

    physical_page_count = NUMBER_OF_PHYSICAL_PAGES;

    // Note: You can treat this variable as an array, each index holds a frame number which we have to associate to our pfn
    // We associate each pfn to a frame number in this array and put it into our free list
    vmState.physical_page_numbers = malloc (physical_page_count * sizeof (ULONG_PTR));

    if (vmState.physical_page_numbers == NULL) {
        printf ("full_virtual_memory_test : could not allocate array to hold physical page numbers\n");
        return;
    }

    allocated = AllocateUserPhysicalPages (physical_page_handle,
                                           &physical_page_count,
                                           vmState.physical_page_numbers);

    if (allocated == FALSE) {
        printf ("full_virtual_memory_test : could not allocate physical pages\n");
        return;
    }

    if (physical_page_count != NUMBER_OF_PHYSICAL_PAGES) {

        printf ("full_virtual_memory_test : allocated only %llu pages out of %u pages requested\n",
                physical_page_count,
                NUMBER_OF_PHYSICAL_PAGES);
    }

    //
    // Reserve a user address space region using the Windows kernel
    // AWE (address windowing extensions) APIs.
    //
    // This will let us connect physical pages of our choosing to
    // any given virtual address within our allocated region.
    //
    // We deliberately make this much larger than physical memory
    // to illustrate how we can manage the illusion.
    //

    virtual_address_size = 64 * physical_page_count * PAGE_SIZE;

    //
    // Round down to a PAGE_SIZE boundary.
    //

    virtual_address_size &= ~PAGE_SIZE;

    vmState.virtual_address_size_in_unsigned_chunks =
                        virtual_address_size / sizeof (ULONG_PTR);

#if SUPPORT_MULTIPLE_VA_TO_SAME_PAGE


    //
    // Allocate a MEM_PHYSICAL region that is "connected" to the AWE section
    // created above.
    //

    vmState.parameter.Type = MemExtendedParameterUserPhysicalHandle;
    vmState.parameter.Handle = physical_page_handle;
    vmState.vaBase = VirtualAlloc2 (NULL,
                       NULL,
                       virtual_address_size,
                       MEM_RESERVE | MEM_PHYSICAL,
                       PAGE_READWRITE,
                       &vmState.parameter,
                       1);

#endif

    if (vmState.vaBase == NULL) {

        printf ("full_virtual_memory_test : could not reserve memory %x\n",
                GetLastError ());

        return;
    }

    //
    // Now perform random accesses.
    // TODO: Turn these into initialize functions
    // Initialize our transfer VA
    // Initialize the array of transfer VAs
    vmState.userTransferVAs = malloc(NUM_OF_USER_THREADS * sizeof(ULONG_PTR));
    for (int i = 0; i < NUM_OF_USER_THREADS; i++) {
        vmState.userTransferVAs[i] = VirtualAlloc2 (NULL,
                       NULL,
                       PAGE_SIZE,
                       MEM_RESERVE | MEM_PHYSICAL,
                       PAGE_READWRITE,
                       &vmState.parameter,
                       1);
    }
    vmState.trimmerTransferVAs = malloc(NUM_OF_TRIMMER_THREADS * sizeof(ULONG_PTR));
    for (int i = 0; i < NUM_OF_TRIMMER_THREADS; i++) {
        vmState.trimmerTransferVAs[i] = VirtualAlloc2 (NULL,
                       NULL,
                       PAGE_SIZE,
                       MEM_RESERVE | MEM_PHYSICAL,
                       PAGE_READWRITE,
                       &vmState.parameter,
                       1);
    }
    vmState.writerTransferVAs = malloc(NUM_OF_WRITER_THREADS * sizeof(ULONG_PTR));
    for (int i = 0; i < NUM_OF_WRITER_THREADS; i++) {
        vmState.writerTransferVAs[i] = VirtualAlloc2 (NULL,
                       NULL,
                       PAGE_SIZE,
                       MEM_RESERVE | MEM_PHYSICAL,
                       PAGE_READWRITE,
                       &vmState.parameter,
                       1);
    }
    // Find the largest frame number
    ULONG64 largestFN = 0;
    for (int i = 0; i < physical_page_count; i++) {
        if (largestFN < vmState.physical_page_numbers[i] + 1) {
            largestFN = vmState.physical_page_numbers[i] + 1;
        }
    }
    // Sparse array of PFN
    vmState.PFN_array = VirtualAlloc(NULL, largestFN * sizeof(PFN), MEM_RESERVE, PAGE_READWRITE);
    if (vmState.PFN_array == NULL) {
        printf("InitializeVirtualMemory : could not allocate PFN_array\n");
    }
    // Create an array of PFN's
    vmState.pfnarray = malloc(largestFN * sizeof(PFN));
    // Error check to see if pfnarray has been allocated
    if (vmState.pfnarray == NULL) {
        printf ("full_virtual_memory_test : could not allocate pfnarray\n");
        return;
    }
    //
    memset(vmState.pfnarray, 0, largestFN * sizeof(PFN));
    initialize_lists (vmState.physical_page_numbers, vmState.pfnarray, physical_page_count);
    initializeSparseArray(vmState.physical_page_numbers);
    // Sets up an array of PTE's called the page table
    vmState.pageTable = malloc(VIRTUAL_ADDRESS_SIZE / PAGE_SIZE * sizeof(PTE));
    if (vmState.pageTable == NULL) {
        printf ("full_virtual_memory_test : could not allocate pageTable\n");
        return;
    }
    ULONG64 numOfPTEs = virtual_address_size / PAGE_SIZE;
    memset(vmState.pageTable,0,numOfPTEs * sizeof(PTE));
    for (int i = 0; i < numOfPTEs; i++) {
        vmState.pageTable[i].DiskFormat.diskIndex = 0;
        vmState.pageTable[i].DiskFormat.mustBeZero = 0;
    }
    // Initialize disk space to continue our illusion
    initialize_disk_space();
}

//
