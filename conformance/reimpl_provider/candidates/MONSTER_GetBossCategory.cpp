// D2MOO_REIMPL_EXPORT: MONSTER_GetBossCategory
#include "../provider_runtime.h"

struct UnitAny {
    uint32_t dwType;
    uint32_t dwTxtFileNo;
};

extern "C" uint32_t __stdcall MONSTER_GetBossCategory(UnitAny* pUnit)
{
    uint32_t dwClassId;
    if ((pUnit != (UnitAny*)0x0) && (pUnit->dwType == 1)) {
        dwClassId = pUnit->dwTxtFileNo;
        if (dwClassId < 0x168) {
            if (dwClassId == 0x167) {
                return 3;
            }
            if (dwClassId == 0x10f) {
                return 1;
            }
            if (dwClassId == 0x152) {
                return 2;
            }
        }
        else if ((0x22f < dwClassId) && (dwClassId < 0x232)) {
            return 4;
        }
    }
    return 0;
}
