#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: PATH_GetNthTarget
extern "C" uint32_t __stdcall PATH_GetNthTarget(void* pPath, int nTargetIndex) {
    if (pPath == nullptr) return 0;

    void* pPathTargetNode = *(void**)((char*)pPath + 0x8);
    uint32_t nValidIndex = 0;

    while (pPathTargetNode != nullptr) {
        if (*(uint32_t*)((char*)pPathTargetNode + 0x4) == 1u) {
            if (nValidIndex == (uint32_t)nTargetIndex) {
                uint16_t hi = (uint16_t)((uint32_t)pPathTargetNode >> 16);
                uint16_t lo = *(uint16_t*)((char*)pPathTargetNode + 0);
                return ((uint32_t)hi << 16) | (uint32_t)lo;
            }
            nValidIndex++;
        }
        pPathTargetNode = *(void**)((char*)pPathTargetNode + 0x8);
    }

    return 0xFFFFu;
}
