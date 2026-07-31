#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_IsPlayerActionStatePending
extern "C" int __stdcall CLIENT_IsPlayerActionStatePending(int nUnitType, int nUnitGuid)
{
    char* base = (char*)*(void**)D2MOO_Resolve("g_pCurrentUnit");
    if (!base)
        return 0;

    /* g_pCurrentUnit->dwFlags == 0  (offset 0x00 in D2UnitStrc) */
    if (*(int*)(base + 0x00) != 0)
        return 0;

    /* iVar1 = g_pCurrentUnit->pPlayerData  (offset 0x14 in D2UnitStrc) */
    int iVar1 = *(int*)(base + 0x14);
    if (iVar1 == 0)
        return 0;

    /* *(int*)(iVar1 + 0x154) == 0x13 */
    if (*(int*)(iVar1 + 0x154) != 0x13)
        return 0;
    /* *(int*)(iVar1 + 0x158) == nUnitType */
    if (*(int*)(iVar1 + 0x158) != nUnitType)
        return 0;
    /* *(int*)(iVar1 + 0x15c) == nUnitGuid */
    if (*(int*)(iVar1 + 0x15c) != nUnitGuid)
        return 0;

    return 1;
}
