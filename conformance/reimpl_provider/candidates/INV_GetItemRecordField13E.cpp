#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: INV_GetItemRecordField13E

extern "C" void __stdcall INV_GetItemRecordField13E(void* pInventory, int nValue)
{
    if (pInventory == nullptr) {
        return;
    }
    if (*(unsigned int*)pInventory == 0x1020304) {
        *(int*)((char*)pInventory + 0x24) = nValue;
    }
}
