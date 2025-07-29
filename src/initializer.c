//
// Created by diego.villegas on 7/9/2025.
#include "../include/initializer.h"
#include "../macros.h"
#include "../include/lists.h"
// Initialize both active and free list and add pfns to free list
void initialize_lists (PULONG_PTR physical_page_numbers, PPFN pfnarray, ULONG_PTR physical_page_count) {
    // Create the head of our active list
    InitializeListHead(&activeList);
    // Create the Head of our free list
    InitializeListHead(&freeList);
    // Create the head of our modified list
    InitializeListHead(&modifiedList);
    // Create the head of our standby list
    InitializeListHead(&standbyList);
}
void initializeSparseArray(PULONG_PTR physical_page_numbers) {
    PPFN pfn;
    for (int i = 0; i < NUMBER_OF_PHYSICAL_PAGES; i++){
        // Commit physical memory to this spot in the PFN array since we will be mapping the frame number to this
        // specific pfn
        LPVOID result = VirtualAlloc((PFN_array + physical_page_numbers[i]), sizeof(PFN),
            MEM_COMMIT, PAGE_READWRITE);
        if (result == NULL) {
            printf("initialize_sparse_array: VirtualAlloc failed\n");
        }
        pfn = PFN_array + physical_page_numbers[i];
        pfn->status = PFN_FREE;
        // TODO: check in about zeroing out this page
        add_entry(&freeList, pfn);
    }
}

void initialize_disk_space() {
    // VA space - PFN space ex: 10 virtual pages - 3 PFN pages = 7 disk pages, having 10 would be a waste
    // We want 7 in this example so that we can have enough space to do swapping
    // We add PAGE_SIZE to help us swap a disk page and physical page when all disk pages are full
    disk_size = VIRTUAL_ADDRESS_SIZE - (NUMBER_OF_PHYSICAL_PAGES * PAGE_SIZE) + (PAGE_SIZE * 2);
    disk = malloc(disk_size);
    if (disk == NULL) {
        printf("initialize_disk_space : disk_space malloc failed");
    }
    memset(disk, 0, disk_size);
    disk_pages = malloc(disk_size / PAGE_SIZE);
    if (disk_pages == NULL) {
        printf("initialize_disk_space : disk_page malloc failed");
    }
    // 0 means that the disk page is available, 1 means the disk page is in use
    memset(disk_pages, 0, disk_size / PAGE_SIZE);
    // Skip index 0 so that when we page fault, we can correctly check diskIndex in PTE invalid format
    disk_page_index = 1;
}

void initializeEvents() {
    startEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
    startTrimmer = CreateEvent(NULL, FALSE, FALSE, NULL);
    finishTrimmer = CreateEvent(NULL, FALSE, FALSE, NULL);
    exitEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
    startWriter = CreateEvent (NULL, FALSE, FALSE, NULL);
    finishWriter = CreateEvent (NULL, FALSE, FALSE, NULL);
}
void initializeThreads() {
    userThreads = malloc(sizeof(HANDLE) * NUM_OF_USER_THREADS);
    for (int i = 0; i < NUM_OF_USER_THREADS; i++) {
        userThreads[i] = CreateThread (DEFAULT_SECURITY,
                               DEFAULT_STACK_SIZE,
                               accessVirtualMemory,
                               NULL,
                               DEFAULT_CREATION_FLAGS,
                               NULL);
        if (userThreads[i] == NULL) {
            DebugBreak();
            printf ("could not create user thread\n");
        }
    }
    trimmerThreads = malloc(sizeof(HANDLE) * NUM_OF_TRIMMER_THREADS);
    for (int i = 0; i < NUM_OF_TRIMMER_THREADS; i++) {
        trimmerThreads[i] = CreateThread (DEFAULT_SECURITY,
                               DEFAULT_STACK_SIZE,
                               trimPage,
                               NULL,
                               DEFAULT_CREATION_FLAGS,
                               NULL);
        if (trimmerThreads[i] == NULL) {
            DebugBreak();
            printf ("could not create trimmer thread\n");
        }
    }
    writerThreads = malloc(sizeof(HANDLE) * NUM_OF_WRITER_THREADS);
    for (int i = 0; i < NUM_OF_WRITER_THREADS; i++) {
        writerThreads[i] = CreateThread (DEFAULT_SECURITY,
                               DEFAULT_STACK_SIZE,
                               writeToDisk,
                               NULL,
                               DEFAULT_CREATION_FLAGS,
                               NULL);
        if (writerThreads[i] == NULL) {
            DebugBreak();
            printf ("could not create trimmer thread\n");
        }
    }
}
void initializeLocks() {
    InitializeCriticalSection(&bigLock);
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
    physical_page_numbers = malloc (physical_page_count * sizeof (ULONG_PTR));

    if (physical_page_numbers == NULL) {
        printf ("full_virtual_memory_test : could not allocate array to hold physical page numbers\n");
        return;
    }

    allocated = AllocateUserPhysicalPages (physical_page_handle,
                                           &physical_page_count,
                                           physical_page_numbers);

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

    virtual_address_size_in_unsigned_chunks =
                        virtual_address_size / sizeof (ULONG_PTR);

#if SUPPORT_MULTIPLE_VA_TO_SAME_PAGE

    MEM_EXTENDED_PARAMETER parameter = { 0 };

    //
    // Allocate a MEM_PHYSICAL region that is "connected" to the AWE section
    // created above.
    //

    parameter.Type = MemExtendedParameterUserPhysicalHandle;
    parameter.Handle = physical_page_handle;

    p = VirtualAlloc2 (NULL,
                       NULL,
                       virtual_address_size,
                       MEM_RESERVE | MEM_PHYSICAL,
                       PAGE_READWRITE,
                       &parameter,
                       1);

#else

    vaBase = VirtualAlloc (NULL,
                      virtual_address_size,
                      MEM_RESERVE | MEM_PHYSICAL,
                      PAGE_READWRITE);

#endif

    if (vaBase == NULL) {

        printf ("full_virtual_memory_test : could not reserve memory %x\n",
                GetLastError ());

        return;
    }

    //
    // Now perform random accesses.
    // TODO: Turn these into initialize functions
    // Initialize our transfer VA
    transfer_va = VirtualAlloc (NULL,
                      PAGE_SIZE,
                      MEM_RESERVE | MEM_PHYSICAL,
                      PAGE_READWRITE);
    // Find the largest frame number
    ULONG64 largestFN = 0;
    for (int i = 0; i < physical_page_count; i++) {
        if (largestFN < physical_page_numbers[i] + 1) {
            largestFN = physical_page_numbers[i] + 1;
        }
    }
    // Sparse array of PFN
    PFN_array = VirtualAlloc(NULL, largestFN * sizeof(PFN), MEM_RESERVE, PAGE_READWRITE);
    if (PFN_array == NULL) {
        printf("InitializeVirtualMemory : could not allocate PFN_array\n");
    }
    // Create an array of PFN's
    pfnarray = malloc(largestFN * sizeof(PFN));
    // Error check to see if pfnarray has been allocated
    if (pfnarray == NULL) {
        printf ("full_virtual_memory_test : could not allocate pfnarray\n");
        return;
    }
    //
    memset(pfnarray, 0, largestFN * sizeof(PFN));
    initialize_lists (physical_page_numbers, pfnarray, physical_page_count);
    initializeSparseArray(physical_page_numbers);
    // Sets up an array of PTE's called the page table
    pageTable = malloc(VIRTUAL_ADDRESS_SIZE / PAGE_SIZE * sizeof(PTE));
    if (pageTable == NULL) {
        printf ("full_virtual_memory_test : could not allocate pageTable\n");
        return;
    }
    ULONG64 numOfPTEs = virtual_address_size / PAGE_SIZE;
    memset(pageTable,0,numOfPTEs * sizeof(PTE));
    for (int i = 0; i < numOfPTEs; i++) {
        pageTable[i].invalidFormat.diskIndex = 0;
        pageTable[i].invalidFormat.mustBeZero = 0;
    }
    // Initialize disk space to continue our illusion
    initialize_disk_space();
}

//
