#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: INV_IsItemTypeInInventory

extern "C" int __stdcall INV_IsItemTypeInInventory(void* pUnit, unsigned int dwItemType) {
    unsigned int* puVar1;

    if (pUnit != nullptr && *(unsigned int*)((char*)pUnit + 0x0) == 0x1020304) {
        puVar1 = (unsigned int*)((char*)pUnit + 0x2c);
        while (puVar1 != (unsigned int*)0x0) {
            if (*puVar1 == dwItemType) {
                return 1;
            }
            puVar1 = (unsigned int*)((char*)puVar1 + 0x4);
        }
    }
    return 0;
}
