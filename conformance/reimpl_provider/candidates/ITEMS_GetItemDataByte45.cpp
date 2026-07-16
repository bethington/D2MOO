// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataByte45
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall ITEMS_GetItemDataByte45(void* pUnit)
{
    if (pUnit != 0) {
        if (*(uint32_t*)pUnit == 4u) {
            void* pItemData = *(void**)((uint8_t*)pUnit + 0x14);
            if (pItemData != 0) {
                return (uint32_t)*(uint8_t*)((uint8_t*)pItemData + 0x45);
            }
        }
    }
    return 0xFFu;
}
