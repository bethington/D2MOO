// D2MOO_REIMPL_EXPORT: STATLIST_GetStatListCount
#include "../provider_runtime.h"

struct UnitAny;
struct StatList;

extern "C" uint32_t __stdcall STATLIST_GetStatListCount(UnitAny* pUnit)
{
    if (pUnit == 0)
        return 0;

    StatList* pStatList = *(StatList**)((uint8_t*)pUnit + 0x5C);
    if (pStatList == 0)
        return 0;

    short count = *(short*)((uint8_t*)pStatList + 0x4C);
    return (uint32_t)(int)count;
}
