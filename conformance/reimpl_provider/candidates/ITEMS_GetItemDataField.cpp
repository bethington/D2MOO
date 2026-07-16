// D2MOO_REIMPL_EXPORT: ITEMS_GetItemDataField
#include "../provider_runtime.h"
#include <stdlib.h>

struct UnitAny;
struct ItemData;

extern "C" uint32_t __stdcall ITEMS_GetItemDataField(UnitAny *pUnit)
{
    if (pUnit != 0 && ((uint32_t *)pUnit)[0] == 4)
    {
        ItemData *pItemData = *(ItemData **)((uint8_t *)pUnit + 0x14);
        if (pItemData != 0)
        {
            return *(uint32_t *)((uint8_t *)pItemData + 0xC);
        }
    }
    _exit(-1);
}
