#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SetViewportCenterOffsets
// NOTE: register_explicit ABI -- Y comes in ECX (the named parameter), X lives in EAX
//       (the decompile's "in_EAX" phantom). The function stores both to globals.
extern "C" void __fastcall SetViewportCenterOffsets(uint32_t dwViewportY)
{
    uint32_t dwViewportX;
    __asm { mov dwViewportX, eax };

    // NEEDS GLOBAL: g_dwViewportCenterDeltaX
    // NEEDS GLOBAL: g_dwViewportCenterY
    void* pX = D2MOO_Resolve("g_dwViewportCenterDeltaX");
    void* pY = D2MOO_Resolve("g_dwViewportCenterY");
    if (!pX || !pY) return;
    *(uint32_t*)pX = dwViewportX;
    *(uint32_t*)pY = dwViewportY;
}
