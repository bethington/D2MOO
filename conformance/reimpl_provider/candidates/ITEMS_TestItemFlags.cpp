// D2MOO_REIMPL_EXPORT: ITEMS_TestItemFlags
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall ITEMS_TestItemFlags(uint32_t pUnit, uint32_t dwFlagMask, uint32_t a3, uint32_t a4) {
    if (pUnit != 0) {
        if (*(uint32_t*)pUnit == 4) {
            uint32_t pItemData = *(uint32_t*)(pUnit + 0x14);
            if (pItemData != 0) {
                return *(uint32_t*)(pItemData + 0x18) & dwFlagMask;
            }
        }
    }
    return 0;
}
