// D2MOO_REIMPL_EXPORT: STAT_GetStatListOwnerId
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall STAT_GetStatListOwnerId(void* pStatList)
{
    if (pStatList == (void*)0x0) {
        return 0;
    }
    return *(uint32_t*)((uint32_t)pStatList + 0x18);
}
