// D2MOO_REIMPL_EXPORT: STATS_GetSlotElementPtr_5920
#include "../provider_runtime.h"

extern "C" void* __cdecl STATS_GetSlotElementPtr_5920(uint32_t nIndex, void* pStatsCtx) {
    if (nIndex >= 0xC) {
        // Defensive guard: original would call CleanupAndAbort + _exit.
        // Game functions are forbidden in the reimpl; return a safe sentinel
        // (this branch is unreachable in real gameplay -- index >= 12 is a bug).
        return (void*)0;
    }
    return (uint8_t*)pStatsCtx + 0xC + (nIndex << 4);
}
