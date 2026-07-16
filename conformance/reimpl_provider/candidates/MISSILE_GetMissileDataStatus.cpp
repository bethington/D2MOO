// D2MOO_REIMPL_EXPORT: MISSILE_GetMissileDataStatus
#include "../provider_runtime.h"

struct ItemData;

struct UnitAny {
    uint32_t dwType;                 // 0x00
    char         _pad0[0x14 - 4];        // pad to 0x14
    union {
        ItemData* pItemData;             // 0x14
    } field5_0x14;
};

struct ItemData {
    char         _pad0[0x28];            // pad to 0x28
    uint32_t dwQuality2;             // 0x28
};

extern "C" uint32_t __stdcall MISSILE_GetMissileDataStatus(UnitAny* pUnit)
{
    if (pUnit != (UnitAny*)0 &&
        pUnit->dwType == 3 &&
        pUnit->field5_0x14.pItemData != (ItemData*)0) {
        return pUnit->field5_0x14.pItemData->dwQuality2;
    }
    return 0;
}
