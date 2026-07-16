// D2MOO_REIMPL_EXPORT: STATS_GetMatrixElementPtr
#include "../provider_runtime.h"
#include <stdlib.h>

extern "C" void* __cdecl STATS_GetMatrixElementPtr(
    uint32_t nRow,        // EAX: row index (must be < 3)
    void* pStatCtx,           // ECX: STATS_MatrixCtx*
    uint32_t dwField,     // EDX: column/field index (must be < 3)
    uint32_t nSavedEsi)   // ESI: PUSH/POP save slot, not used semantically
{
    (void)nSavedEsi;

    // Bounds check field index (matches CMP EDX,0x3 / JC valid path)
    if (dwField >= 3u) {
        // Original: PUSH 0x8F; CALL CleanupAndAbort (never returns)
        _exit(-1);
    }
    // Bounds check row index (matches CMP EAX,0x3 / JNC valid path)
    if (nRow >= 3u) {
        // Original: PUSH 0x90; CALL CleanupAndAbort (never returns)
        _exit(-1);
    }

    // Original builds stack[0..2] = &pStatCtx->pColBase0/1/2 (the ADDRESSES,
    // not the values), then does MOV ECX,[ESP+EDX*4]; SHL EAX,4; ADD EAX,ECX.
    // So return = (char*)pStatCtx + 0x30 + dwField*0x30 + (nRow << 4).
    return (void*)((uint8_t*)pStatCtx + 0x30u + dwField * 0x30u + (nRow << 4));
}
