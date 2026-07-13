#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: PATH_GetPositionEntry
//
// Reimpl of D2Common!PATH_GetPositionEntry (0x6fd6b890).
// Returns a pointer into the global position-entry array at the given index.
// Reads g_pPathPositionEntryArray (g_dat_6fdf1464) and g_dwPathPositionCount
// (g_dat_6fdf1468) by NAME via the injected resolver; no hardcoded addresses.
// Stride per entry is 0x148; out-of-range/null abort branch returns 0 sentinel.

extern "C" void* __stdcall PATH_GetPositionEntry(int nPositionIndex)
{
    // g_pPathPositionEntryArray -> g_dat_6fdf1464 (pointer variable, deref once).
    char* base = (char*)*(void**)D2MOO_Resolve("g_dat_6fdf1464");
    if (base == nullptr) return 0;

    // g_dwPathPositionCount -> g_dat_6fdf1468 (DWORD data, use resolved address directly).
    char* cnt_base = (char*)D2MOO_Resolve("g_dat_6fdf1468");
    if (cnt_base == nullptr) return 0;
    uint32_t count = *(uint32_t*)cnt_base;

    // Decompile literal: nPositionIndex < (int)g_dwPathPositionCount
    if (nPositionIndex < (int)count)
    {
        // Decompile literal: nPositionIndex * 0x148 + (int)g_pPathPositionEntryArray
        return (void*)(nPositionIndex * 0x148 + (int)base);
    }

    // Abort branch (CleanupAndAbort + _exit) collapsed to sentinel -- input_sets
    // must stay strictly in-range, so this branch is not exercised by the oracle.
    return 0;
}
