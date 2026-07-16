// D2MOO_REIMPL_EXPORT: UNIT_GetWeaponStyle
#include "../provider_runtime.h"

// UNIT_GetWeaponStyle @ 0x6fd73bc0
// stdcall, 1 stack arg (pUnit), ret in EAX, RET 0x4
// Returns the dword at offset 0x10 of the unit-data pointer, gated on:
//   pUnit != NULL && pUnit->dwType == 4 && pUnit->pData != NULL
// returns 0 in all other cases.
extern "C" uint32_t __stdcall UNIT_GetWeaponStyle(void* pUnit)
{
    if (pUnit == 0) return 0;
    if (*(int*)pUnit != 4) return 0;
    void* pData = *(void**)((char*)pUnit + 0x14);
    if (pData == 0) return 0;
    return *(uint32_t*)((char*)pData + 0x10);
}
