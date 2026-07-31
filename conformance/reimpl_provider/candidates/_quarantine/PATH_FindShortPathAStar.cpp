// PATH_FindShortPathAStar -- A* short-path orchestrator.
// Reproduces D2Common 0x6fdcb630. Reads pPathParams (a 48-byte path-search
// context struct per plate comment) and uses register-explicit incoming args
// (EBX/ESI/EDI). Calls D2Common internal PATH_* callees (linked from D2Common.dll).
#include "../provider_runtime.h"

// D2Common internal PATH_* callees (linked at build time)
extern "C" {
    uint8_t __stdcall PATH_CheckAdjacentPassable(void);
    void __stdcall PATH_InsertNodeIntoHashAndPriority(void* node, void* ctx);
    void* __stdcall PATH_MoveNodeToClosedSet(void);
    int __stdcall PATH_ExpandAllNeighbors(void* pPathParams, void* openList, uint32_t targetCoord);
    uint32_t __stdcall PATH_BuildWaypointArray(void* node);
}

// D2MOO_REIMPL_EXPORT: PATH_FindShortPathAStar
extern "C" uint32_t __stdcall PATH_FindShortPathAStar(
    void* pPathParams,    /* EBX: path-search context (start/target coord, collision ctx, masks) */
    void* esi_ctx,        /* ESI: register arg passed through to PATH_* */
    void* edi_ctx         /* EDI: register arg (start-node buffer) passed through to PATH_* */
)
{
    /* Step 1: Quick fail - verify movement is possible between start/target. */
    if (!PATH_CheckAdjacentPassable())
        return 0;

    /* Step 2: Allocate stack buffers (mirrors __alloca_probe of 0x3300):
         dwOpenList   - 128 DWORDS (512 bytes)
         dwClosedList - 128 DWORDS (512 bytes)
         dwStartCoordStack - 14 DWORDS (56 bytes, path-node struct)
       Each is zero-initialized (the decompile's three memset loops). */
    uint32_t dwOpenList[128];
    uint32_t dwClosedList[128];
    uint32_t dwStartCoordStack[14];

    for (int i = 0; i < 128; i++) dwOpenList[i] = 0;
    for (int i = 0; i < 128; i++) dwClosedList[i] = 0;
    for (int i = 0; i < 14;  i++) dwStartCoordStack[i] = 0;

    /* Step 3: Read packed X/Y coords (low16=X, hi16=Y) from pPathParams. */
    uint32_t uVar1           = *(uint32_t*)pPathParams;
    uint32_t dwNeighborContext = *(uint32_t*)((char*)pPathParams + 4);

    /* Step 4: Manhattan-style heuristic with diagonal weighting.
       abs(dx) / abs(dy) - when |dx|<|dy| the result = |dx| + 2*|dy|,
       else |dy| + 2*|dx|. Stored to sStack_2ef0 (offset 0x10 in start node). */
    int dx = (int)(uVar1 & 0xFFFF) - (int)(dwNeighborContext & 0xFFFF);
    if (dx < 0) dx = -dx;
    int dy = (int)(uVar1 >> 0x10) - (int)(dwNeighborContext >> 0x10);
    if (dy < 0) dy = -dy;

    short sStack_2ef0;
    if (dx < dy) {
        sStack_2ef0 = (short)dx + (short)dy * 2;
    } else {
        sStack_2ef0 = (short)dy + (short)dx * 2;
    }

    /* Step 5: Populate start node struct (g=0 at start, so h == f).
       +0x00 = packed coord, +0x10 = h-score, +0x12 = f-score. */
    dwStartCoordStack[0] = uVar1;
    *((short*)((char*)dwStartCoordStack + 0x10)) = sStack_2ef0;
    *((short*)((char*)dwStartCoordStack + 0x12)) = sStack_2ef0;

    /* Step 6: Insert start node into the hash table and priority queue. */
    PATH_InsertNodeIntoHashAndPriority(edi_ctx, esi_ctx);

    /* Step 7: Main A* loop - pop from priority queue, tie-break, expand neighbors.
       Tie-break (g+5 bias): prefer lower wGScore; on ties prefer
       node whose stored parent X-coordinate is shallower (pNode->wParentX + 5 < pPVar2->wParentX). */
    void* pNode = 0;
    void* pPopped = PATH_MoveNodeToClosedSet();
    if (pPopped == 0)
        return 0;

    do {
        short wGScorePopped = *(short*)((char*)pPopped + 0x10);
        short wParentXPopped = *(short*)((char*)pPopped + 0x14);

        short wGScoreNode = 0;
        short wParentXNode = 0;
        if (pNode != 0) {
            wGScoreNode = *(short*)((char*)pNode + 0x10);
            wParentXNode = *(short*)((char*)pNode + 0x14);
        }

        if (pNode == 0 ||
            wGScorePopped < wGScoreNode ||
            (wGScorePopped == wGScoreNode && (int)wParentXNode + 5 < (int)wParentXPopped)) {
            pNode = pPopped;
        }

        if (wGScorePopped == 0) break;
        int expandRes = PATH_ExpandAllNeighbors(pPathParams, (void*)dwOpenList, dwNeighborContext);
        if (expandRes == 0) break;
        pPopped = PATH_MoveNodeToClosedSet();
    } while (pPopped != 0);

    /* Step 8: If a node was reached, build waypoint array; else fail. */
    if (pNode != 0) {
        return PATH_BuildWaypointArray(pNode);
    }
    return 0;
}
