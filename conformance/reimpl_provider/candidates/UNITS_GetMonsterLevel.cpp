#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: UNITS_GetMonsterLevel

extern "C" uint32_t __fastcall UNITS_GetMonsterLevel(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    int dwUnitType = *(int*)pUnit;

    if ((dwUnitType != 2) && ((dwUnitType <= 3) || (dwUnitType > 5)))
    {
        /* Non-Object/Item/Warp: return 16-bit wPosY (MOVZX zero-extends) */
        void* pPath = *(void**)((char*)pUnit + 0x2c);
        if (pPath == nullptr) return 0;
        return (uint32_t)*(uint16_t*)((char*)pPath + 0x6);
    }

    /* Object(2)/Item(4)/Warp(5): return 32-bit dwPosPair */
    return *(uint32_t*)((char*)(*(void**)((char*)pUnit + 0x2c)) + 0x10);
}
