// D2MOO_REIMPL_EXPORT: STAT_GetUnitCalculatedStat
#include "../provider_runtime.h"

/* STAT_GetUnitCalculatedStat @ 0x6fd7eaf0 (stdcall, RET 0x4, EAX = result)
 * FAST-PATH two-level getter: [pUnit+0x14] then [+0x2c] -> dwItemLevel.
 * Gating: pUnit != NULL && *(uint32_t*)pUnit == 0  (i.e. pUnit->dwType == 0).
 * On miss, the real function calls GetReturnAddress/CleanupAndAbort/_exit(-1)
 * with error codes 0xfc9 (null pUnit) or 0xfca (dwType != 0). The shadow
 * dispatcher must not terminate the game on that path, so we return 0; the
 * miss path is unreachable in normal real-gameplay shadow runs. */
extern "C" uint32_t __stdcall STAT_GetUnitCalculatedStat(void* pUnit)
{
    if (pUnit != (void*)0x0 && *(const uint32_t*)pUnit == 0u) {
        const void* pItemData = *(void* const*)((const char*)pUnit + 0x14);
        return *(const uint32_t*)((const char*)pItemData + 0x2c);
    }
    return 0u;
}
