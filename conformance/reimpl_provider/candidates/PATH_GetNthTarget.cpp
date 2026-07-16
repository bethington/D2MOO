// D2MOO_REIMPL_EXPORT: PATH_GetNthTarget
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall PATH_GetNthTarget(void* pPath, int nTargetIndex) {
    /* dwTargetNode from pPath + 0x08 (live game object). */
    uint32_t node = *(uint32_t*)((uint8_t*)pPath + 0x08u);
    int nValidIndex = 0;
    for (;;) {
        if (node == 0u) {
            /* Sentinel: end-of-list / no target found => low 16 bits all ones. */
            return 0xFFFFu;
        }
        if (*(int*)((uint8_t*)(uintptr_t)node + 0x04u) == 1) {
            if (nValidIndex == nTargetIndex) {
                /* Upper 16 bits = high word of node pointer (preserved from EAX);
                   lower 16 bits = node->wGUIDLow at offset 0x00. */
                uint16_t wLow = *(uint16_t*)(uintptr_t)node;
                return (node & 0xFFFF0000u) | (uint32_t)wLow;
            }
            ++nValidIndex;
        }
        /* Advance to next linked-list node at offset 0x08. */
        node = *(uint32_t*)((uint8_t*)(uintptr_t)node + 0x08u);
    }
}
