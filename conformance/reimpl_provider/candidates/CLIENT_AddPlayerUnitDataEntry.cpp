#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_AddPlayerUnitDataEntry
extern "C" void __stdcall CLIENT_AddPlayerUnitDataEntry(uint32_t pPlayerUnit)
{
    uint32_t* pdwSlots;
    uint32_t dwIndex;

    // &g_pPlayerUnitSlots is the base of the 8-slot array (per plate comment:
    // "Array of 8 uint slots at 0x6FBCB800"). The decompile uses &g_pPlayerUnitSlots
    // (address-of) -- the resolver returns the address of the symbol directly, so
    // we do NOT deref here. (Treading this as a g_p pointer would read slot[0]'s
    // value and walk 8 uints from there, which is wrong.)
    char* base = (char*)D2MOO_Resolve("g_pPlayerUnitSlots");
    if (!base)
        return; // resolver missing / symbol unknown -> silent fail

    pdwSlots = (uint32_t*)base;
    dwIndex = 0;
    do {
        if (*pdwSlots == 0) {
            *pdwSlots = pPlayerUnit;
            return;
        }
        pdwSlots = pdwSlots + 1;
        dwIndex = dwIndex + 1;
    } while ((int)dwIndex < 8);
    return;
}
