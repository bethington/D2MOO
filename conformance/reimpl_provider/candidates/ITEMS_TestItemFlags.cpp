#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_TestItemFlags
extern "C" uint32_t __stdcall ITEMS_TestItemFlags(
    void* pUnit,
    uint32_t dwFlagMask,
    int unused3,
    int unused4)
{
    if (pUnit == nullptr) return 0;

    // pUnit->dwType must be 4 (item)
    if (*(uint32_t*)pUnit != 4u) return 0;

    // pUnit->pItemData at offset 0x14
    void* pItemData = *(void**)((char*)pUnit + 0x14);
    if (pItemData == nullptr) return 0;

    // pItemData->dwFlags at offset 0x18
    uint32_t dwFlags = *(uint32_t*)((char*)pItemData + 0x18);
    return dwFlags & dwFlagMask;
}
