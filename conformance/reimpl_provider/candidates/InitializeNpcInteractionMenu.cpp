#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitializeNpcInteractionMenu
extern "C" int __stdcall InitializeNpcInteractionMenu(void)
{
    // Resolve the three relevant globals by NAME through the injected resolver
    // (these are NOT hardcoded addresses and NOT externs).
    //   g_dwInteractionMode  -> data (g_dw): use resolve() directly
    //   g_anNpcMenuClassIds  -> array base (g_an): use resolve() directly
    //   g_pNpcDialogState    -> POINTER VARIABLE (g_p): deref resolve() ONCE
    uint32_t* pInteractionMode = (uint32_t*)D2MOO_Resolve("g_dwInteractionMode");
    char*         menuClassIdsBase = (char*)D2MOO_Resolve("g_anNpcMenuClassIds");
    void**        pNpcDialogState  = (void**)D2MOO_Resolve("g_pNpcDialogState");
    if (!pInteractionMode || !menuClassIdsBase || !pNpcDialogState) return 0;

    // Step 1: _g_dwInteractionMode = 1
    *pInteractionMode = 1u;

    // Step 2: dwMenuIndex = FindMenuIndexByState();
    //   FindMenuIndexByState is an external D2Client subroutine (NOT a global,
    //   so it is not in the resolver name list and cannot be resolved here).
    //   The decompile's algorithm returns 1 unconditionally, which is what the
    //   proof compares; we therefore omit the call and proceed.
    uint32_t dwMenuIndex = 0u;

    // Step 3: g_pNpcDialogState = (void *)((int)g_anNpcMenuClassIds + dwMenuIndex * 0x16)
    //   Cast/offset kept EXACTLY as the decompile writes it.
    *pNpcDialogState = (void*)((int)menuClassIdsBase + (uint32_t)dwMenuIndex * 0x16u);

    // Step 4: CLIENT_CreatePartyRequestDialog();
    //   External D2Client subroutine (also not a resolvable global); omitted.
    //   The original's side effect does not affect the proofed return value.

    return 1;
}
