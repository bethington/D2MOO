#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: MONSTER_GetMaxComponentVisualTier

extern "C" uint32_t __stdcall MONSTER_GetMaxComponentVisualTier(void* pUnit, uint32_t dwBitMask) {
    void* pItemData;
    if (pUnit != nullptr
        && *(int*)pUnit == 4
        && (pItemData = *(void**)((char*)pUnit + 0x14)) != nullptr) {
        return *(uint32_t*)((char*)pItemData + 0x14) & dwBitMask;
    }
    return 0;
}
