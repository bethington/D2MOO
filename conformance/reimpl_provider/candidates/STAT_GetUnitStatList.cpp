// D2MOO_REIMPL_EXPORT: STAT_GetUnitStatList
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall STAT_GetUnitStatList(void* pUnit)
{
    return *(uint32_t*)((int)pUnit + 0x3c);
}
