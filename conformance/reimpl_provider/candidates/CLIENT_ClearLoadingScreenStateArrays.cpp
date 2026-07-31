#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_ClearLoadingScreenStateArrays
extern "C" void __stdcall CLIENT_ClearLoadingScreenStateArrays()
{
    // Resolve the base pointer for the loading screen state buffer array
    uint32_t* pBase = (uint32_t*)D2MOO_Resolve("g_dwAmbientSoundState");
    if (!pBase)
        return;

    // Iterate 0x400 (1024) times, zeroing each uint entry in the state buffer
    uint32_t* pEntry = pBase;
    uint32_t dwCount = 0x400;
    while (dwCount != 0) {
        *pEntry = 0;
        pEntry = pEntry + 1;
        dwCount = dwCount - 1;
    }

    // Reset the various global sound state fields to their initial values
    uint32_t* pIdleTick = (uint32_t*)D2MOO_Resolve("g_dwAmbientIdleSoundLastTick");
    if (pIdleTick)
        pIdleTick[0] = 0;

    uint32_t* pLastSoundTs = (uint32_t*)D2MOO_Resolve("g_dwLastSoundTimestamp");
    if (pLastSoundTs)
        *pLastSoundTs = 0;

    // Magic byte 0x5A marker / sentinel / validation value
    uint32_t* pCooldownExpiry = (uint32_t*)D2MOO_Resolve("g_dwAmbientSoundCooldownExpiry");
    if (pCooldownExpiry)
        *pCooldownExpiry = 0x5A;

    uint32_t* pField4CC = (uint32_t*)D2MOO_Resolve("g_dwSoundStateField4CC");
    if (pField4CC)
        *pField4CC = 0;

    uint32_t* pLastSoundIdx = (uint32_t*)D2MOO_Resolve("g_dwLastSoundIndex");
    if (pLastSoundIdx)
        *pLastSoundIdx = 0;
}
