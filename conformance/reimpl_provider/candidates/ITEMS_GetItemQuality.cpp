#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemQuality
// int __stdcall(pItem) -> *(int*)pItemData (quality at pItemData+0x00), else 2 (Normal).
// pItemData is the union member at pUnit+0x14, gated on dwType==4 (item). Self-contained.
// Verified from disasm 0x6fd73b40: MOV EAX,[ESP+4]; CMP [EAX],4; MOV EAX,[EAX+0x14]; MOV EAX,[EAX]; RET 4.

extern "C" int __stdcall ITEMS_GetItemQuality(void* pUnit)
{
    if (pUnit != nullptr && *(unsigned int*)pUnit == 4) {
        void* pItemData = *(void**)((char*)pUnit + 0x14);
        if (pItemData != nullptr) {
            return *(int*)pItemData;
        }
    }
    return 2;
}
