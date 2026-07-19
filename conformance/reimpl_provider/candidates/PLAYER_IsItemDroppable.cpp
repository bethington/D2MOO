#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: PLAYER_IsItemDroppable
extern "C" int __stdcall PLAYER_IsItemDroppable(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    // Check if this is an item (type == 4) with the "cannot be dropped" path flag
    int dwType = *(int*)((char*)pUnit + 0x0);
    int* pItemData = *(int**)((char*)pUnit + 0x14);
    if (dwType == 4 && pItemData != (int*)0 &&
        (*(int*)((char*)pItemData + 0x18) & 0x1000) != 0) {
        return 0;
    }

    // Resolve and call through to the real ITEMS_IsItemNormalDroppable (stdcall, 1 stack arg)
    typedef void* (__stdcall *ITEMS_IsItemNormalDroppable_t)(uint32_t);
    ITEMS_IsItemNormalDroppable_t _f =
        (ITEMS_IsItemNormalDroppable_t)D2MOO_Resolve("ITEMS_IsItemNormalDroppable");
    if (_f == nullptr) return 0;

    uint32_t dwUnitClassId = *(uint32_t*)((char*)pUnit + 0x4);
    void* _r = _f(dwUnitClassId);

    if ((int)(intptr_t)_r == 0) return 0;
    return 1;
}
