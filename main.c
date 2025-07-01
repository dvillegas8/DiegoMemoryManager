#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "macros.h"

//
// This define enables code that lets us create multiple virtual address
// mappings to a single physical page.  We only/need want this if/when we
// start using reference counts to avoid holding locks while performing
// pagefile I/Os - because otherwise disallowing this makes it easier to
// detect and fix unintended failures to unmap virtual addresses properly.
//

#define SUPPORT_MULTIPLE_VA_TO_SAME_PAGE 0

#pragma comment(lib, "advapi32.lib")

#if SUPPORT_MULTIPLE_VA_TO_SAME_PAGE
#pragma comment(lib, "onecore.lib")
#endif

#define PAGE_SIZE                   4096

#define MB(x)                       ((x) * 1024 * 1024)

//
// This is intentionally a power of two so we can use masking to stay
// within bounds.
//

#define VIRTUAL_ADDRESS_SIZE        MB(16)

#define VIRTUAL_ADDRESS_SIZE_IN_UNSIGNED_CHUNKS        (VIRTUAL_ADDRESS_SIZE / sizeof (ULONG_PTR))

//
// Deliberately use a physical page pool that is approximately 1% of the
// virtual address space
//

#define NUMBER_OF_PHYSICAL_PAGES   ((VIRTUAL_ADDRESS_SIZE / PAGE_SIZE) / 64)

BOOL
GetPrivilege  (
    VOID
    )
{
    struct {
        DWORD Count;
        LUID_AND_ATTRIBUTES Privilege [1];
    } Info;

    //
    // This is Windows-specific code to acquire a privilege.
    // Understanding each line of it is not so important for
    // our efforts.
    //

    HANDLE hProcess;
    HANDLE Token;
    BOOL Result;

    //
    // Open the token.
    //

    hProcess = GetCurrentProcess ();

    Result = OpenProcessToken (hProcess,
                               TOKEN_ADJUST_PRIVILEGES,
                               &Token);

    if (Result == FALSE) {
        printf ("Cannot open process token.\n");
        return FALSE;
    }

    //
    // Enable the privilege.
    //

    Info.Count = 1;
    Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Get the LUID.
    //

    Result = LookupPrivilegeValue (NULL,
                                   SE_LOCK_MEMORY_NAME,
                                   &(Info.Privilege[0].Luid));

    if (Result == FALSE) {
        printf ("Cannot get privilege\n");
        return FALSE;
    }

    //
    // Adjust the privilege.
    //

    Result = AdjustTokenPrivileges (Token,
                                    FALSE,
                                    (PTOKEN_PRIVILEGES) &Info,
                                    0,
                                    NULL,
                                    NULL);

    //
    // Check the result.
    //

    if (Result == FALSE) {
        printf ("Cannot adjust token privileges %u\n", GetLastError ());
        return FALSE;
    }

    if (GetLastError () != ERROR_SUCCESS) {
        printf ("Cannot enable the SE_LOCK_MEMORY_NAME privilege - check local policy\n");
        return FALSE;
    }

    CloseHandle (Token);

    return TRUE;
}

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

VOID
malloc_test (
    VOID
    )
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


VOID
commit_at_fault_time_test (
    VOID
    )
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

// Pte = Page Table Entry
// Struct has 64 bits available
// Note, the number all the way to the right is how many bits we allocate to the field
typedef struct {
    // Bit that tells us that pte is active, meaning pte maps to a valid physical page
    ULONG64 valid: 1;
    // Page frame number, maps to an actual physical page, 40 bits because we can shave off 12 bits from the
    // physical address. If we want the physical address back, we multiply by 4k
    ULONG64 frameNumber: 40;
} validPte;

typedef struct {
    // Bit that tell us that this pte is not active, meaning pte is not mapped into physical memory
    ULONG64 mustBeZero: 1;
    // Helps us locate the contents of the physical page that are saved onto disk because the page
    // might have become a victim of our trimming (creating a free page)
    ULONG64 diskIndex: 40;
    // How many bits we have left
    ULONG64 reserve: 23;
} invalidPte;

typedef struct {
    // Since we make so many pte, we want to union the fields to save space (Union allows multiple variables to
    // share the same memory space) = overlay
    union {
        validPte validFormat;
        invalidPte invalidFormat;
        // **Check in with Landy about this**, not sure what this is for
        ULONG64 entireFormat;
    };
} PTE, *PPTE;

typedef struct {
    // Help us create our doubly linked list
    LIST_ENTRY entry;
    // Pointer of the PTE this pfn belongs to
    PPTE pte;
    // Index that maps to a place in physical memory
    ULONG64 frameNumber;

} PFN, *PPFN;

// Our doubly linked list containing all of our free pages
LIST_ENTRY freeList;
// Our doubly linked list containing all of our active pages
LIST_ENTRY activeList;
LIST_ENTRY modifiedList;
// Virtual address...array? We can use this to find the beginning/base of our virtual address
PULONG_PTR p;
// PTE array called pageTable
PPTE pageTable;
// Index that has access to a disk page that is available
ULONG64 disk_page_index;
// Array of booleans to show us which disk pages are available
boolean* disk_pages;
// Disk
PVOID disk;
// Space for disk
ULONG_PTR disk_size;
// Used to help us write to disk
PULONG_PTR transfer_va;



// Method which adds a new entry into our free list
void add_entry(PLIST_ENTRY head, PFN* newpfn) {
    PLIST_ENTRY previousNewestEntry = head->Blink;
    // Makes new entry/pfn Blink point towards the entry before it
    newpfn->entry.Blink = previousNewestEntry;
    // Make the "entry before" Flink point towards our new entry/pfn
    previousNewestEntry->Flink = &newpfn->entry;
    // Make the new entry/pfn Flink point towards the head
    newpfn->entry.Flink = head;
    // Make the head Blink point towards our new entry/pfn
    head->Blink = &newpfn->entry;
}
// Get a free page from the free list
PPFN pop_page(PLIST_ENTRY head) {
    // Check for empty list
    if (head->Flink == head) {
        printf("pop_page : empty list");
        return NULL;
    }
    // Get the page/PFN we want to remove
    PPFN freePage = (PPFN)head->Flink;
    // Get the PFN after our freepage
    PPFN nextPage = (PPFN)freePage->entry.Flink;
    // Make head Flink point towards nextpage
    head->Flink = &nextPage->entry;
    // Make nextpage blink point towards head
    nextPage->entry.Blink = head;
    return freePage;
}
// Initialize both active and free list and add pfns to free list
void initialize_lists (PULONG_PTR physical_page_numbers, PPFN pfnarray, ULONG_PTR physical_page_count) {
    // Create the head of our active list
    InitializeListHead(&activeList);
    // Create the Head of our free list
    InitializeListHead(&freeList);
    // Create the head of our modified list
    InitializeListHead(&modifiedList);
    // Get the head of our free list
    PLIST_ENTRY head = &freeList;
    PPFN pfn;
    // Add all pages to free list
    for (int i = 0; i < physical_page_count; i++) {
        pfn = &pfnarray[i];
        // Save the frame number of a certain physical page into it's PFN
        pfn->frameNumber = physical_page_numbers[i];
        // Add pfn into our doubly linked list
        add_entry(head, pfn);
    }
}

PPTE va_to_pte(PULONG_PTR virtual_address) {
    ULONG64 index = ((ULONG64) virtual_address - (ULONG64) p) / PAGE_SIZE;
    PPTE pte = pageTable + index;
    return pte;
}
PULONG_PTR pte_to_va(PPTE pte) {
    ULONG64 index = pte - pageTable;
    PULONG_PTR va = (PULONG_PTR)((index * PAGE_SIZE) + (ULONG64) p);
    return va;
}
// Get a page from the active list
PPFN find_victim(PLIST_ENTRY head) {
    if (head->Flink == head) {
        printf("find_victim : empty list");
        return NULL;
    }
    // Grab the last page in the list
    PPFN victim = (PPFN)head->Blink;
    // Grab the 2nd to last page
    PPFN pageBefore = (PPFN)victim->entry.Blink;
    // Make 2nd to last page point towards head
    pageBefore->entry.Flink = head;
    // Make head blink point towards 2nd to last page
    head->Blink = &pageBefore->entry;
    return victim;
}

void initialize_disk_space() {
    // VA space - PFN space ex: 10 virtual pages - 3 PFN pages = 7 disk pages, having 10 would be a waste
    // We want 7 in this example so that we can have enough space to do swapping
    // We add PAGE_SIZE to help us swap a disk page and physical page when all disk pages are full
    disk_size = VIRTUAL_ADDRESS_SIZE - (NUMBER_OF_PHYSICAL_PAGES * PAGE_SIZE) + PAGE_SIZE;
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
// Look for a free disk index
boolean write_to_disk(ULONG64 frameNumber) {
    // Temporarily map physical page to the transfer_va
    if (MapUserPhysicalPages(transfer_va, 1, &frameNumber) == FALSE) {
        DebugBreak();
        printf("write_to_disk : transfer_va is not mapped\n");
    }
    ULONG64 counter = 0;
    // Look for a disk_page that is available
    while(disk_pages[disk_page_index] != 0 && counter != 2) {
        disk_page_index++;
        // Check if we are at the end of the array
        if (disk_page_index == (disk_size / PAGE_SIZE) - 1){
            // Wrap around the disk
            disk_page_index = 1;
            counter++;
        }
    }
    // Means that there is no disk page available
    if (counter >= 2) {
        MapUserPhysicalPages(transfer_va, 1, NULL);
        return FALSE;
    }
    PVOID diskAddress = (PVOID)((ULONG64) disk + disk_page_index * PAGE_SIZE);
    // Copy contents from transfer va to diskAddress
    memcpy(diskAddress, transfer_va, PAGE_SIZE);
    // Make disk page unavailable
    disk_pages[disk_page_index] = 1;
    // Unmap transfer VA
    if (MapUserPhysicalPages(transfer_va, 1, NULL) == FALSE) {
        DebugBreak();
        printf("write_to_disk : transfer_va could not be unmapped");
    }
    return TRUE;
}
void read_disk(ULONG64 disk_index, ULONG64 frameNumber) {
    PVOID diskAddress = (PVOID)((ULONG64) disk + (disk_index * PAGE_SIZE));
    if (MapUserPhysicalPages(transfer_va, 1, &frameNumber) == FALSE) {
        DebugBreak();
        printf("read_to_disk : transfer_va could not be mapped");
    }
    memcpy(transfer_va, diskAddress, PAGE_SIZE);
    if (MapUserPhysicalPages(transfer_va, 1, NULL) == FALSE) {
        DebugBreak();
        printf("read_to_disk : transfer_va could not be unmapped");
    }
    // Make disk page available
    disk_pages[disk_index] = 0;
}
void zeroPage(ULONG64 frameNumber) {
    if (MapUserPhysicalPages(transfer_va, 1, &frameNumber) == FALSE) {
        DebugBreak();
        printf("read_to_disk : transfer_va could not be mapped");
    }
    memset(transfer_va, 0, PAGE_SIZE);
    if (MapUserPhysicalPages(transfer_va, 1, NULL) == FALSE) {
        DebugBreak();
        printf("read_to_disk : transfer_va could not be unmapped");
    }
}
void trim_page() {
    // Get the "oldest page" off the active list & remove it
    PPFN victim = find_victim(&activeList);
    if (victim == NULL) {
        printf("trim_page : victim from find_victim() is empty");
    }
    PPTE pte = victim->pte;
    if (pte == NULL) {
        printf("trim_page : victim pte is empty");
    }
    PULONG_PTR virtual_address = pte_to_va(pte);
    // Unmap pte
    if (MapUserPhysicalPages(virtual_address, 1, NULL) == FALSE) {
        DebugBreak();
        printf("trim_page : VA could not be unmapped");
    }
    // write to disc
    if (write_to_disk(victim->frameNumber)){
        // update pte in pfn
        victim->pte->invalidFormat.mustBeZero = 0;
        victim->pte->invalidFormat.diskIndex = disk_page_index;
    }
    else {
        victim->pte->entireFormat = 0;
    }
    add_entry(&freeList, victim);
}
VOID
checkVa(PULONG64 va) {
    va = (PULONG64) ((ULONG64)va & ~(PAGE_SIZE - 1));
    for (int i = 0; i < PAGE_SIZE / 8; ++i) {
        if (!(*va == 0 || *va == (ULONG64) va)) {
            DebugBreak();
        }
        va += 1;
    }
}



VOID
full_virtual_memory_test (
    VOID
    )
{
    unsigned i;
    PULONG_PTR arbitrary_va;
    unsigned random_number;
    BOOL allocated;
    BOOL page_faulted;
    BOOL privilege;
    BOOL obtained_pages;
    ULONG_PTR physical_page_count;
    // Array which holds all the frame numbers
    PULONG_PTR physical_page_numbers;
    HANDLE physical_page_handle;
    ULONG_PTR virtual_address_size;
    ULONG_PTR virtual_address_size_in_unsigned_chunks;

    //
    // Allocate the physical pages that we will be managing.
    //
    // First acquire privilege to do this since physical page control
    // is typically something the operating system reserves the sole
    // right to do.
    //

    privilege = GetPrivilege ();

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

    p = VirtualAlloc (NULL,
                      virtual_address_size,
                      MEM_RESERVE | MEM_PHYSICAL,
                      PAGE_READWRITE);

#endif

    if (p == NULL) {

        printf ("full_virtual_memory_test : could not reserve memory %x\n",
                GetLastError ());

        return;
    }

    //
    // Now perform random accesses.
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
    // Create an array of PFN's
    PPFN pfnarray = malloc(largestFN * sizeof(PFN));
    // Error check to see if pfnarray has been allocated
    if (pfnarray == NULL) {
        printf ("full_virtual_memory_test : could not allocate pfnarray\n");
        return;
    }
    //
    memset(pfnarray, 0, largestFN * sizeof(PFN));
    initialize_lists (physical_page_numbers, pfnarray, physical_page_count);
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

        random_number = rand () * rand () * rand ();

        random_number %= virtual_address_size_in_unsigned_chunks;

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

        arbitrary_va = p + random_number;

        __try {

            *arbitrary_va = (ULONG_PTR) arbitrary_va;

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            page_faulted = TRUE;
        }

        if (page_faulted) {

            //
            // Connect the virtual address now - if that succeeds then
            // we'll be able to access it from now on.
            //
            // THIS IS JUST REUSING THE SAME PHYSICAL PAGE OVER AND OVER !
            //
            // IT NEEDS TO BE REPLACED WITH A TRUE MEMORY MANAGEMENT
            // STATE MACHINE !
            //
            PLIST_ENTRY head = &freeList;
            // Check if Free list is empty
            if(IsListEmpty(head)){
                trim_page();
            }
            // Get the PTE from the va
            PPTE pte = va_to_pte(arbitrary_va);
            PPFN freePage;
            // Get a free page from the free list
            freePage = pop_page(head);
            if (freePage == NULL) {
                printf("full_virtual_memory_test: freePage is null");
            }
            // Get frame number for PFN/free page
            ULONG64 frameNumber = freePage->frameNumber;
            // Check if we saved the contents in disk
            if (pte->invalidFormat.diskIndex != 0) {
                read_disk(pte->invalidFormat.diskIndex, frameNumber);
            }
            // VA that hasn't been connected to a physical page before case
            else {
                // Erase the old contents of the physical page
                zeroPage(frameNumber);
            }
            // save the pte the pfn corresponds to
            freePage->pte = pte;
            // Update PTE to reflect what I did (later when we have to look for victims), store 40 bit frame number & set valid to 1
            freePage->pte->validFormat.frameNumber = frameNumber;
            freePage->pte->validFormat.valid = 1;
            // get Head of active list and add our "free page" (which is now active) into our active list
            head = &activeList;
            add_entry(head, freePage);
            // Map arbitrary_va to frame number
            if (MapUserPhysicalPages (arbitrary_va, 1, &frameNumber) == FALSE) {
                DebugBreak();
                printf ("full_virtual_memory_test : could not map VA %p to page %llX\n", arbitrary_va, *physical_page_numbers);

                return;
            }

            //
            // No exception handler needed now since we have connected
            // the virtual address above to one of our physical pages
            // so no subsequent fault can occur.
            //

            *arbitrary_va = (ULONG_PTR) arbitrary_va;
            checkVa(arbitrary_va);
        }
    }

    printf ("full_virtual_memory_test : finished accessing %u random virtual addresses\n", i);

    //
    // Now that we're done with our memory we can be a good
    // citizen and free it.
    //

    VirtualFree (p, 0, MEM_RELEASE);

    return;
}

VOID
main (
    int argc,
    char** argv
    )
{
    //
    // Test a simple malloc implementation - we call the operating
    // system to pay the up front cost to reserve and commit everything.
    //
    // Page faults will occur but the operating system will silently
    // handle them under the covers invisibly to us.
    //
#if 0
    malloc_test ();

    //
    // Test a slightly more complicated implementation - where we reserve
    // a big virtual address range up front, and only commit virtual
    // addresses as they get accessed.  This saves us from paying
    // commit costs for any portions we don't actually access.  But
    // the downside is what if we cannot commit it at the time of the
    // fault !
    //

    commit_at_fault_time_test ();
#endif

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
    // This is where we can be as creative as we like, the sky's the limit !
    //

    full_virtual_memory_test ();

    return;
}