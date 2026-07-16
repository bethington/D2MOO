// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataField64
#include "../provider_runtime.h"

struct UnitAny_s {
    uint32_t dwType;        // +0x00
    uint32_t _pad04[4];     // +0x04 .. +0x13
    void*    pItemData;     // +0x14
};

extern "C" uint32_t __stdcall ITEMS_GetItemDataField64(UnitAny_s* pUnit)
{
    void* pItemData;
    if (pUnit != (UnitAny_s*)0
        && pUnit->dwType == 4u
        && (pItemData = pUnit->pItemData, pItemData != (void*)0)
        && pItemData != (void*)0xFFFFFFA4) {
        return *(uint32_t*)((uintptr_t)pItemData + 0x64u);
    }
    return 0u;
}
