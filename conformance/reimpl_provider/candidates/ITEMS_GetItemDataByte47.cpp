#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataByte47

extern "C" unsigned char __stdcall ITEMS_GetItemDataByte47(void* pUnit)
{
    if (pUnit == nullptr) {
        return 0xFF;
    }
    if (*(unsigned int*)((char*)pUnit + 0x0) != 4) {
        return 0xFF;
    }
    void* pItemData = *(void**)((char*)pUnit + 0x14);
    if (pItemData == nullptr) {
        return 0xFF;
    }
    return *(unsigned char*)((char*)pItemData + 0x47);
}
