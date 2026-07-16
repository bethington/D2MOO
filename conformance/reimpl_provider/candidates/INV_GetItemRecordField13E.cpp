// D2MOO_REIMPL_EXPORT: INV_GetItemRecordField13E
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall INV_GetItemRecordField13E(uint32_t* pInventory, uint32_t nValue)
{
    /* 0x1de70  10268  */
    if ((pInventory != (uint32_t*)0) && (pInventory[0] == 0x1020304)) {
        pInventory[9] = nValue;  /* offset 0x24 = index 9 */
    }
    return (uint32_t)pInventory;
}
