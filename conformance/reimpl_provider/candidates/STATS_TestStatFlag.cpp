#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STATS_TestStatFlag
// NEEDS GLOBAL: g_pdwStatBitMasks

extern "C" uint32_t __fastcall STATS_TestStatFlag(void* pUnit, uint32_t dwFlagIndex)
{
    if (pUnit == 0) return 0;

    void* pStatList = *(void**)((char*)pUnit + 0x5C);
    if (pStatList == 0) return 0;

    int validity = *(int*)((char*)pStatList + 0x10);
    if (validity >= 0) return 0;

    void* flagArrayBase = *(void**)((char*)pStatList + 0x38);

    uint32_t dwordIndex = (uint32_t)((int)dwFlagIndex >> 5);
    uint32_t bitIndex = dwFlagIndex & 0x1Fu;

    uint32_t flagDword = *(uint32_t*)((char*)flagArrayBase + dwordIndex * 4u);

    char* maskBase = (char*)D2MOO_Resolve("g_pdwStatBitMasks");
    if (!maskBase) return 0xDEADBEEFu;

    uint32_t bitMask = *(uint32_t*)(maskBase + bitIndex * 4u);

    return flagDword & bitMask;
}
