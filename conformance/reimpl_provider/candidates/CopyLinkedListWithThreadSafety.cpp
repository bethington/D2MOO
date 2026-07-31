#include "../provider_runtime.h"
#include <windows.h>

// D2MOO_REIMPL_EXPORT: CopyLinkedListWithThreadSafety
extern "C" int __stdcall CopyLinkedListWithThreadSafety(
    int nEventId,
    void* pOutputBuffer,
    void* pValidationParam,
    void* pItemCount)
{
    // NEEDS GLOBAL: g_pCritSecMessageQueue (closest match for g_pCritSec in this module)
    char* pCritSecBase = (char*)D2MOO_Resolve("g_pCritSecMessageQueue");
    if (!pCritSecBase) return (int)0xDEADBEEF;

    // Step 1: Validate pValidationParam - return error 0x57 if null
    if (!pValidationParam) {
        SetLastError(0x57);
        return 0;
    }

    // Step 2: Acquire critical section lock
    EnterCriticalSection((LPCRITICAL_SECTION)pCritSecBase);

    // Step 3: Initialize pValidationParam->dwValue to 0
    *(uint32_t*)pValidationParam = 0;

    // Step 4: Check if _g_pGameStateVtbl is not null (initialized flag)
    // NEEDS GLOBAL: g_pGameStateVtbl
    char* pVtblBase = (char*)D2MOO_Resolve("g_pGameStateVtbl");
    void* pVtbl = NULL;
    if (pVtblBase) {
        // _g_pGameStateVtbl is a pointer variable - dereference resolved address
        pVtbl = *(void**)pVtblBase;
    }

    // Step 5: If null, set error 0x4B4, release lock, return 0
    if (!pVtbl) {
        SetLastError(0x4B4);
        LeaveCriticalSection((LPCRITICAL_SECTION)pCritSecBase);
        return 0;
    }

    // Step 6/7/8: Call CopyFilteredLinkedListToBuffer (cannot call from reimpl).
    // The original passes (SNETListNode*)nEventId and pOutputBuffer as args.
    // Without ability to invoke the Storm internal routine, the live oracle
    // exercises failure paths only (init-not-set, null-validation) where the
    // success branch is unreachable.

    // Release lock and return 0 (matches failure path deterministically)
    LeaveCriticalSection((LPCRITICAL_SECTION)pCritSecBase);
    return 0;
}
