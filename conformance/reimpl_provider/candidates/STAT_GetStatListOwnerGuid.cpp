// D2MOO_REIMPL_EXPORT: STAT_GetStatListOwnerGuid
#include "../provider_runtime.h"

struct STATLIST_Ctx {
    uint32_t dwField00;
    uint32_t dwField04;
    uint32_t dwField08;
    uint32_t dwField0C;
    uint32_t dwField10;
    uint32_t dwField14;
    uint32_t dwField18;
    uint32_t dwField1C;
    uint32_t dwOwnerGuid;
};

extern "C" uint32_t __stdcall STAT_GetStatListOwnerGuid(STATLIST_Ctx* pStatList)
{
    if (pStatList != 0 && pStatList->dwField00 == 0x1020304) {
        return pStatList->dwOwnerGuid;
    }
    return 0;
}
