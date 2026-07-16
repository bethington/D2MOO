// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataByte69
#include "../provider_runtime.h"

struct UnitAny {
    uint32_t dwType;          // +0x00
    uint8_t  _pad[0x14 - 0x04];
    void*    pItemData;       // +0x14
};

extern "C" uint32_t __stdcall ITEMS_GetItemDataByte69(UnitAny* pUnit)
{
    if (pUnit == nullptr) return 0;
    if (pUnit->dwType != 4) return 0;
    uint8_t* pItemData = (uint8_t*)pUnit->pItemData;
    if (pItemData == nullptr) return 0;
    if (pItemData == (uint8_t*)0xffffffa4) return 0;
    int8_t b = (int8_t)pItemData[0x69];
    return (uint32_t)(int32_t)b;
}
