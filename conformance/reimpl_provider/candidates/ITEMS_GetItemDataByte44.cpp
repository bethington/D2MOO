// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataByte44
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall ITEMS_GetItemDataByte44(void* pUnit) {
    if (pUnit != 0 &&
        *(uint32_t*)pUnit == 4 &&
        *(void**)((uint8_t*)pUnit + 0x14) != 0) {
        uint8_t* pItemData = *(uint8_t**)((uint8_t*)pUnit + 0x14);
        return (uint32_t)pItemData[0x44];
    }
    return 0;
}
