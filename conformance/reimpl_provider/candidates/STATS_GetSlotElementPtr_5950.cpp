// D2MOO_REIMPL_EXPORT: STATS_GetSlotElementPtr_5950
#include "../provider_runtime.h"
#include <stdlib.h>

extern "C" void* __cdecl STATS_GetSlotElementPtr_5950(uint32_t in_EAX, void* pStatsCtx)
{
    if (pStatsCtx != (void*)0 && in_EAX < 3u) {
        return (void*)((uintptr_t)pStatsCtx + 0x24u + (in_EAX << 4));
    }
    /* Error path mirrors disasm: never returns. Both real and shadow die here. */
    _exit(-1);
}
