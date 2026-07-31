#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_UpdateServerSyncFrameTimeout
// Mirrors the decompiled behavior:
//   dwCurrentTick = GetTickCount();
//   if (g_dwServerSyncTimeoutTick < dwCurrentTick + dwTimeoutDelta)
//       g_dwServerSyncTimeoutTick = dwCurrentTick + dwTimeoutDelta;
// g_dwServerSyncTimeoutTick is a direct data global (name starts g_dw, not g_p)
// so D2MOO_Resolve already returns &g_dwServerSyncTimeoutTick.
extern "C" void __stdcall CLIENT_UpdateServerSyncFrameTimeout(uint32_t dwTimeoutDelta)
{
    // Resolve the deadline global by verified name.
    uint32_t* pTimeout = (uint32_t*)D2MOO_Resolve("g_dwServerSyncTimeoutTick");
    if (!pTimeout)
        return;

    // Resolve GetTickCount. g_pfnGetTickCount is a function pointer variable
    // (name starts g_p), so the decompile's bare reference is its VALUE --
    // deref the resolved address ONCE to get the callable pointer.
    void* getTickCountAddr = *(void**)D2MOO_Resolve("g_pfnGetTickCount");
    if (!getTickCountAddr)
        return;

    typedef uint32_t (*GetTickCountFn_t)(void);
    GetTickCountFn_t pfnGetTickCount = (GetTickCountFn_t)getTickCountAddr;

    uint32_t dwCurrentTick = pfnGetTickCount();
    uint32_t dwNewDeadline = dwCurrentTick + dwTimeoutDelta;

    // Match the decompile's unsigned compare + extend-on-greater exactly.
    if (*pTimeout < dwNewDeadline)
    {
        *pTimeout = dwNewDeadline;
    }
}
