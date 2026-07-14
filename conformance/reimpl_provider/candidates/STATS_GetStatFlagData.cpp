#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STATS_GetStatFlagData
extern "C" uint32_t __fastcall STATS_GetStatFlagData(void* p) {
    if (p == nullptr) return 0;
    void* pStatFlagCtx = *(void**)((char*)p + 0x5C);
    if (pStatFlagCtx == nullptr) return 0;
    int nFlagCondition = *(int*)((char*)pStatFlagCtx + 0x10);
    if (nFlagCondition < 0) {
        return *(uint32_t*)((char*)pStatFlagCtx + 0x58);
    }
    return 0;
}
