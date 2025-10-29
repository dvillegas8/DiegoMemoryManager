#include"../include/main.h"

#include "dthreads.h"



//
// This define enables code that lets us create multiple virtual address
// mappings to a single physical page.  We only/need want this if/when we
// start using reference counts to avoid holding locks while performing
// pagefile I/Os - because otherwise disallowing this makes it easier to
// detect and fix unintended failures to unmap virtual addresses properly.
//



#pragma comment(lib, "advapi32.lib")

#if SUPPORT_MULTIPLE_VA_TO_SAME_PAGE
#pragma comment(lib, "onecore.lib")
#endif

#if SUPPORT_MULTIPLE_VA_TO_SAME_PAGE

HANDLE
CreateSharedMemorySection (
    VOID
    )
{
    HANDLE section;
    MEM_EXTENDED_PARAMETER parameter = { 0 };

    //
    // Create an AWE section.  Later we deposit pages into it and/or
    // return them.
    //

    parameter.Type = MemSectionExtendedParameterUserPhysicalFlags;
    parameter.ULong = 0;

    section = CreateFileMapping2 (INVALID_HANDLE_VALUE,
                                  NULL,
                                  SECTION_MAP_READ | SECTION_MAP_WRITE,
                                  PAGE_READWRITE,
                                  SEC_RESERVE,
                                  0,
                                  NULL,
                                  &parameter,
                                  1);

    return section;
}

#endif

VOID malloc_test (VOID)
{
    unsigned i;
    PULONG_PTR p;
    unsigned random_number;

    p = malloc (VIRTUAL_ADDRESS_SIZE);

    if (p == NULL) {
        printf ("malloc_test : could not malloc memory\n");
        return;
    }

    for (i = 0; i < MB (1); i += 1) {

        //
        // Randomly access different portions of the virtual address
        // space we obtained above.
        //
        // If we have never accessed the surrounding page size (4K)
        // portion, the operating system will receive a page fault
        // from the CPU and proceed to obtain a physical page and
        // install a PTE to map it - thus connecting the end-to-end
        // virtual address translation.  Then the operating system
        // will tell the CPU to repeat the instruction that accessed
        // the virtual address and this time, the CPU will see the
        // valid PTE and proceed to obtain the physical contents
        // (without faulting to the operating system again).
        //

        random_number = rand ();

        random_number %= VIRTUAL_ADDRESS_SIZE_IN_UNSIGNED_CHUNKS;

        //
        // Write the virtual address into each page.  If we need to
        // debug anything, we'll be able to see these in the pages.
        //

        *(p + random_number) = (ULONG_PTR) p;
    }

    printf ("malloc_test : finished accessing %u random virtual addresses\n", i);

    //
    // Now that we're done with our memory we can be a good
    // citizen and free it.
    //

    free (p);

    return;
}


VOID commit_at_fault_time_test (VOID)
{
    unsigned i;
    PULONG_PTR p;
    PULONG_PTR committed_va;
    unsigned random_number;
    BOOL page_faulted;

    p = VirtualAlloc (NULL,
                      VIRTUAL_ADDRESS_SIZE,
                      MEM_RESERVE,
                      PAGE_NOACCESS);

    if (p == NULL) {
        printf ("commit_at_fault_time_test : could not reserve memory\n");
        return;
    }
    for (i = 0; i < MB (1); i += 1) {
        //
        // Randomly access different portions of the virtual address
        // space we obtained above.
        //
        // If we have never accessed the surrounding page size (4K)
        // portion, the operating system will receive a page fault
        // from the CPU and proceed to obtain a physical page and
        // install a PTE to map it - thus connecting the end-to-end
        // virtual address translation.  Then the operating system
        // will tell the CPU to repeat the instruction that accessed
        // the virtual address and this time, the CPU will see the
        // valid PTE and proceed to obtain the physical contents
        // (without faulting to the operating system again).
        //

        random_number = rand ();

        random_number %= VIRTUAL_ADDRESS_SIZE_IN_UNSIGNED_CHUNKS;

        //
        // Write the virtual address into each page.  If we need to
        // debug anything, we'll be able to see these in the pages.
        //

        page_faulted = FALSE;

        __try {

            *(p + random_number) = (ULONG_PTR) p;

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            page_faulted = TRUE;
        }

        if (page_faulted) {

            //
            // Commit the virtual address now - if that succeeds then
            // we'll be able to access it from now on.
            //

            committed_va = p + random_number;

            committed_va = VirtualAlloc (committed_va,
                                         sizeof (ULONG_PTR),
                                         MEM_COMMIT,
                                         PAGE_READWRITE);

            if (committed_va == NULL) {
                printf ("commit_at_fault_time_test : could not commit memory\n");
                return;
            }

            //
            // No exception handler needed now since we are guaranteed
            // by virtue of our commit that the operating system will
            // honor our access.
            //

            *committed_va = (ULONG_PTR) committed_va;
        }
    }

    printf ("commit_at_fault_time_test : finished accessing %u random virtual addresses\n", i);

    //
    // Now that we're done with our memory we can be a good
    // citizen and free it.
    //

    VirtualFree (p, 0, MEM_RELEASE);

    return;
}

ULONG accessVirtualMemory (PVOID context)
{
    ULONG ReturnValue;


    ReturnValue = WaitForSingleObject (vmState.startEvent, INFINITE);
    if (ReturnValue == 0) {
        ULONG ExitCode;
        ReturnValue = GetExitCodeProcess (vmState.userThreads, &ExitCode);
    }
    unsigned i;
    PULONG_PTR arbitrary_va;
    unsigned random_number;
    BOOL page_faulted;
    PTHREAD_INFO threadInfo;

    threadInfo = (PTHREAD_INFO)context;
    // This is a loop that accesses random virtual addresses
    // This loop doesn't know anything about the internals of the page fault machinery because we are separating that
    // knowledge. This is a user mode function and our page fault handling is a kernel mode function
    // TODO: Later move accessVirtualMemory into a different file because this is the user mode function, separate
    // TODO: from all page fault machine
    for (i = 0; i < 1000; i += 1){
        //
        // Randomly access different portions of the virtual address
        // space we obtained above.
        //
        // If we have never accessed the surrounding page size (4K)
        // portion, the operating system will receive a page fault
        // from the CPU and proceed to obtain a physical page and
        // install a PTE to map it - thus connecting the end-to-end
        // virtual address translation.  Then the operating system
        // will tell the CPU to repeat the instruction that accessed
        // the virtual address and this time, the CPU will see the
        // valid PTE and proceed to obtain the physical contents
        // (without faulting to the operating system again).
        //

        random_number = rand () * rand () * rand ();

        random_number %= vmState.virtual_address_size_in_unsigned_chunks;

        //
        // Write the virtual address into each page.  If we need to
        // debug anything, we'll be able to see these in the pages.
        //

        page_faulted = FALSE;

        //
        // Ensure the write to the arbitrary virtual address doesn't
        // straddle a PAGE_SIZE boundary just to keep things simple for
        // now.
        //

        random_number &= ~0x7;

        arbitrary_va = vmState.vaBase + random_number;
        while (TRUE) {
            __try {

                *arbitrary_va = (ULONG_PTR) arbitrary_va;

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                page_faulted = TRUE;
            }
            if (!page_faulted) {
                break;
            }
            // the arbitrary va was inaccessible

            // Call operating system to make it appear
            pageFaultHandler(arbitrary_va, threadInfo);
        }
    }
    printf ("full_virtual_memory_test : finished accessing %u random virtual addresses\n", i);
    return 0;
}
//
void tearDownVirtualMemory(){
    // free(pointer) - > function to free mallocs
    // freeuserphysical pages
    // TODO: free all mallocs, virtuallocates, terminate threads,
    VirtualFree(vmState.vaBase, 0, MEM_RELEASE);
    //VirtualFree(transfer_va, 0, MEM_RELEASE);
    free(vmState.physical_page_numbers);
    free(vmState.pfnarray);
    free(vmState.pageTable);

}
VOID
full_virtual_memory_test (
    VOID
    )
{
    ULONG ReturnValue;


    // Set up PTEs, PFNs, disks, threads, events, lists
    initializeEvents();
    initializeLocks();
    initializeVirtualMemory();
    initializeThreads();
    SetEvent(vmState.startEvent);
    // Run user mode accesses to exercise virtual memory
    // Run user mode accesses to exercise virtual memory

    HANDLE userThreadsHandles[NUM_OF_USER_THREADS];
    for (int i = 0; i < NUM_OF_USER_THREADS; i++) {
        userThreadsHandles[i] = vmState.userThreads[i].ThreadHandle;
    }
    ReturnValue = WaitForMultipleObjects (NUM_OF_USER_THREADS, userThreadsHandles, TRUE, INFINITE);
    // ReturnValue = WaitForSingleObject (userThreads[0], INFINITE);
    if (ReturnValue == 0) {

        ULONG ExitCode;

        ReturnValue = GetExitCodeProcess (vmState.userThreads,
                                          &ExitCode);
    }
    SetEvent(vmState.exitEvent);
    printf("exit event fired\n");
    ResetEvent(vmState.exitEvent);
    //
    // Now that we're done with our memory we can be a good
    // citizen and free it.
    //
    tearDownVirtualMemory();
    return;
}

VOID
main (
    int argc,
    char** argv
    )
{
    // TODO:
    //
    // Test our very complicated usermode virtual implementation.
    //
    // We will control the virtual and physical address space management
    // ourselves with the only two exceptions being that we will :
    //
    // 1. Ask the operating system for the physical pages we'll use to
    //    form our pool.
    //
    // 2. Ask the operating system to connect one of our virtual addresses
    //    to one of our physical pages (from our pool).
    //
    // We would do both of those operations ourselves but the operating
    // system (for security reasons) does not allow us to.
    //
    // But we will do all the heavy lifting of maintaining translation
    // tables, PFN data structures, management of physical pages,
    // virtual memory operations like handling page faults, materializing
    // mappings, freeing them, trimming them, writing them out to backing
    // store, bringing them back from backing store, protecting them, etc.
    //

    full_virtual_memory_test ();

    return;
}