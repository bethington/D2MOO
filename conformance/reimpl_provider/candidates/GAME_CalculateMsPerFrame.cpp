#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GAME_CalculateMsPerFrame
extern "C" void __stdcall GAME_CalculateMsPerFrame(void)
{
    // NEEDS GLOBAL: g_dwGameFrameRate
    // NEEDS GLOBAL: g_dwMsPerFrame

    int* pFrameRate = (int*)D2MOO_Resolve("g_dwGameFrameRate");
    uint32_t* pMsPerFrame = (uint32_t*)D2MOO_Resolve("g_dwMsPerFrame");

    // Misconfig sentinel: bail without writing -> obvious mismatch with original.
    if (!pFrameRate || !pMsPerFrame)
        return;

    int frameRate = *pFrameRate;
    // Match decompile exactly: (int) -> longlong -> divide 1000 -> cast to uint.
    uint32_t result = (uint32_t)(1000LL / (int64_t)frameRate);
    *pMsPerFrame = result;
}
