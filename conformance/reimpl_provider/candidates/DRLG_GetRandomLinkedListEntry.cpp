// D2MOO_REIMPL_EXPORT: DRLG_GetRandomLinkedListEntry
#include "../provider_runtime.h"

typedef struct DRLG_FindSpawnPositionNode {
    char _pad[0x24];
    struct DRLG_FindSpawnPositionNode* pNextNode;
} DRLG_FindSpawnPositionNode;

typedef struct DRLG_FindSpawnPositionCtx {
    char _pad0[0x8];
    uint32_t dwMax;
    char _pad1[0x4];
    void* pListHead;
    char _pad2[0x1B0];
    uint64_t rngSeed;
} DRLG_FindSpawnPositionCtx;

// One parameter (EAX = pCtx). EDI/ESI are callee-saved and not consumed by the algorithm.
extern "C" void* __cdecl DRLG_GetRandomLinkedListEntry(void* pCtx) {
    if (pCtx == NULL) {
        // Original: CleanupAndAbort(0x13E); _exit(-1). Shadow harness never hits this branch.
        *(volatile int*)0 = 0;
        return NULL;
    }

    void* pListHead = *(void**)((char*)pCtx + 0x10);
    if (pListHead == NULL) {
        // Original: CleanupAndAbort(0x13F); _exit(-1). Shadow harness never hits this branch.
        *(volatile int*)0 = 0;
        return NULL;
    }

    // -- Inlined SEED_GetRandomNumberAlt(ulonglong* pSeed /*ECX*/, uint dwMax /*ESI*/) --
    uint32_t dwMax = *(uint32_t*)((char*)pCtx + 0x8);
    uint64_t* pSeed = (uint64_t*)((char*)pCtx + 0x1C4);

    uint32_t uVar1;
    if ((int)dwMax < 1) {
        uVar1 = 0;
    } else {
        uint32_t loSeed = *(uint32_t*)pSeed;
        uint32_t hiSeed = *((uint32_t*)pSeed + 1);
        uint64_t newSeed =
            (uint64_t)loSeed * 0x6AC690C5ULL + (uint64_t)hiSeed;
        *(uint64_t*)pSeed = newSeed;
        if ((dwMax & (dwMax - 1)) != 0) {
            uVar1 = (uint32_t)((newSeed & 0xFFFFFFFFULL) % (uint64_t)dwMax);
        } else {
            uVar1 = (dwMax - 1) & (uint32_t)newSeed;
        }
    }

    // -- Loop: walk uVar1 steps via pNextNode at node + 0x24 --
    DRLG_FindSpawnPositionNode* pNode = (DRLG_FindSpawnPositionNode*)pListHead;
    while (uVar1 != 0) {
        uVar1 = uVar1 - 1;
        pNode = (DRLG_FindSpawnPositionNode*)pNode->pNextNode;
    }
    return pNode;
}
