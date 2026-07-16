// D2MOO_REIMPL_EXPORT: STAT_IsUnitStatListFlag8Set
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall STAT_IsUnitStatListFlag8Set(int pUnit)
{
    int pStatList;
    if (pUnit == 0) return 0;
    pStatList = *(int*)(pUnit + 0x5C);
    if (pStatList == 0) return 0;
    return (*(uint32_t*)(pStatList + 0x10) >> 8) & 1u;
}
