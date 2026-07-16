// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataShort36
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall ITEMS_GetItemDataShort36(void* pUnit) {
    if (pUnit != (void*)0 && *(int*)pUnit == 4) {
        int pPath = *(int*)((char*)pUnit + 0x14);
        if (pPath != 0) {
            return (uint32_t)*(uint16_t*)((char*)pPath + 0x36);
        }
    }
    return 0;
}
