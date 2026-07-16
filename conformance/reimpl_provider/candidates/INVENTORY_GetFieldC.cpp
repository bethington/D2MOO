// D2MOO_REIMPL_EXPORT: INVENTORY_GetFieldC
#include "../provider_runtime.h"

struct Inventory {
    uint32_t dwSignature;   // +0x00: magic 0x1020304
    uint32_t pad_0x04;      // +0x04
    uint32_t pad_0x08;      // +0x08
    uint32_t dwField0x0C;   // +0x0C: returned value
};

extern "C" uint32_t __stdcall INVENTORY_GetFieldC(Inventory* pInventory)
{
    if (pInventory != (Inventory*)0 && pInventory->dwSignature == 0x1020304)
    {
        return (uint32_t)pInventory->dwField0x0C;
    }
    return 0;
}
