// D2MOO_REIMPL_EXPORT: DATATBLS_GetCharCompositDualRecord
//
// Reimplementation of D2Common.dll!DATATBLS_GetCharCompositDualRecord (0x6fd68ef0).
//
// Stdcall with 2 stack args (RET 0x8): dwContext (uint32), nCompositIndex (int32).
// Returns a void* pointing into one of two table bases (class or mode), stride
// 0x34 (sizeof(COMPOSIT_DUAL)). Bounds-checks dwContext against the live
// g_dwCharCompositCount and gates on the data-table-initialized flag at
// 0x...13c8. Invalid nCompositIndex (anything other than 0 or 1) returns 0.
// The abort path (iVar1 == 0 after compute, or flag == 0) is collapsed to
// `return 0` because CleanupAndAbort/GetReturnAddress/_exit are not defined
// in this provider TU and would not compile; in-range test inputs never hit
// it in normal loaded game state.
//
// All four globals are resolved BY NAME through D2MOO_Resolve -- never as
// hardcoded addresses or externs (provider is a standalone DLL; extern game
// symbols break the build).

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetCharCompositDualRecord
extern "C" void* __stdcall DATATBLS_GetCharCompositDualRecord(
    uint32_t dwContext,
    int nCompositIndex)
{
    int iVar1 = 0;

    // Resolve the four globals by verified name. The two g_p* names are
    // pointer VARIABLES (their dereferenced value is the record base), so
    // we resolve once and deref once. The two g_dw* are scalars used
    // directly via the resolved address.
    int*    pCount       = (int*)   D2MOO_Resolve("g_dat_6fdf13c4"); // g_dwCharCompositCount
    int*    pFlag        = (int*)   D2MOO_Resolve("g_dat_6fdf13c8"); // g_dwData_13c8 (init flag)
    void**  ppClassBase  = (void**) D2MOO_Resolve("g_dat_6fdf13cc"); // g_pCharCompositClassBase (T*)
    void**  ppModeBase   = (void**) D2MOO_Resolve("g_dat_6fdf13d0"); // g_pCharCompositModeBase  (T*)

    if (!pCount || !pFlag || !ppClassBase || !ppModeBase)
    {
        // Resolver not injected / unknown name -- return an obvious
        // out-of-band sentinel so a misconfig fails loudly rather than
        // matching by accident.
        return (void*)(int)0xDEADBEEF;
    }

    uint32_t count = (uint32_t)*pCount;
    int          flag  = *pFlag;

    if (dwContext < count)
    {
        if (flag == 0)
        {
            // Original calls CleanupAndAbort + _exit(-1) here. The
            // provider TU has no such helpers; collapse to return 0 so
            // the build links. In valid loaded game state this branch
            // is unreachable (flag is non-zero after DataTbls init).
            return (void*)0;
        }

        if (nCompositIndex == 0)
        {
            iVar1 = (int)(dwContext * 0x34u + (int)(*ppClassBase));
        }
        else if (nCompositIndex == 1)
        {
            iVar1 = (int)(dwContext * 0x34u + (int)(*ppModeBase));
        }
        else
        {
            iVar1 = 0;
        }

        if (iVar1 == 0)
        {
            // Same CollapseAndAbort equivalent -- in normal play the
            // resolved table base is non-null, so this branch never
            // computes to 0 for in-range dwContext.
            return (void*)0;
        }
    }
    else
    {
        iVar1 = 0;
    }

    return (void*)iVar1;
}
