#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: IsPointInMenuLeftRegion
extern "C" int __stdcall IsPointInMenuLeftRegion(int nMouseX)
{
    char* base = (char*)D2MOO_Resolve("g_dwScreenWidth");
    if (!base) return 0; // resolver not injected -> obvious wrong-value sentinel

    uint32_t dwScreenWidth = *(uint32_t*)base;
    int nCenterX = (int)(dwScreenWidth - 0x26c) / 2;
    return (nMouseX < nCenterX + 0x12 + ((int)(dwScreenWidth + nCenterX * -2 + -0x31) / 3) * 2) ? 1 : 0;
}
