#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: ITEMS_IsUnitDualWieldClass

extern "C" uint32_t __stdcall ITEMS_IsUnitDualWieldClass(void* pUnit, int nPrefixIndex)
{
    if (pUnit == nullptr) return 0;

    int dwType = *(int*)((char*)pUnit + 0x00);
    if (dwType != 4) return 0;

    void* pItemData = *(void**)((char*)pUnit + 0x14);
    if (pItemData == nullptr) return 0;

    uint16_t result = *(uint16_t*)((char*)pItemData + 0x38 + nPrefixIndex * 2);
    return (uint32_t)result;
}
