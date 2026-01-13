//
// Created by diego.villegas on 7/9/2025.
#include "../include/util.h"
#include "../include/initializer.h"
boolean isValidFrame(PPFN pfn) {
    ULONG64 frameNumber = getFrameNumber(pfn);
    if (frameNumber > (vmState.largestFN)*sizeof(PFN)) {
        return false;
    }
    return true;
}
void checkVa(PULONG64 va) {
    va = (PULONG64) ((ULONG64)va & ~(PAGE_SIZE - 1));
    for (int i = 0; i < PAGE_SIZE / 8; ++i) {
        if (!(*va == 0 || *va == (ULONG64) va)) {
            printf("Check Va error");
            DebugBreak();
        }
        va += 1;
    }
}
void zeroPage(ULONG64 frameNumber, PTHREAD_INFO threadInfo) {
    if (MapUserPhysicalPages(threadInfo->transferVA, 1, &frameNumber) == FALSE) {
        DebugBreak();
        printf("read_to_disk : transfer_va could not be mapped");
    }
    memset(threadInfo->transferVA, 0, PAGE_SIZE);
    if (MapUserPhysicalPages(threadInfo->transferVA, 1, NULL) == FALSE) {
        DebugBreak();
        printf("read_to_disk : transfer_va could not be unmapped");
    }
}
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
void clearDiskSlot(ULONG diskIndex) {
    vmState.disk_pages[diskIndex] = 0;
}

ULONG64 getPTELock(PPTE pte) {
    ULONG64 index;
    index = pte - vmState.pageTable;
    index = index / NUM_OF_PTES_PER_REGION;
    return index;
}
//
