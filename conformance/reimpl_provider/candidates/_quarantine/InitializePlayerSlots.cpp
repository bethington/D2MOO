#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitializePlayerSlots
// NEEDS GLOBAL: g_pExceptionList

// Sub-routine that the original calls; declared so the reimpl links against
// the same in-game implementation that the live proof harness exercises.
extern "C" void __stdcall InitializeGameStructures(void* pCtx);

extern "C" void* __fastcall InitializePlayerSlots(void* pGameObjectCtx)
{
    // g_pExceptionList is a pointer variable; D2MOO_Resolve returns &g_pExceptionList,
    // so a single deref yields the live pointer value (as `_g_pExceptionList` in decompile).
    void** pExceptionList = (void**)D2MOO_Resolve("g_pExceptionList");
    if (!pExceptionList) return 0; // resolver missing -> obvious mismatch sentinel

    // SEH frame setup (mirrors decompile): save previous frame pointer, link new frame.
    void* prev = *pExceptionList;
    *pExceptionList = &prev;

    // Initialize main game object structures (sub-routine; real impl in running game).
    InitializeGameStructures(pGameObjectCtx);

    // Player slot array base at offset +0x3474; 128 slots of 12 bytes (0xC) each.
    uint32_t* pdwPlayerSlot = (uint32_t*)((char*)pGameObjectCtx + 0x3474);
    uint32_t nRemainingSlots = 0x80;
    do {
        *pdwPlayerSlot = 0;            // zero first dword of the 12-byte slot
        pdwPlayerSlot = pdwPlayerSlot + 3; // advance 3 dwords = 12 bytes per slot
        nRemainingSlots = nRemainingSlots - 1;
    } while (nRemainingSlots != 0);

    // SEH teardown: restore previous frame pointer.
    *pExceptionList = prev;

    return pGameObjectCtx;
}
