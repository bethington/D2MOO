#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: IsPointInScrollTopArrowRegion
extern "C" int __fastcall IsPointInScrollTopArrowRegion(
    uint32_t nMouseX,
    uint32_t nMouseY,
    uint32_t nUnused)
{
    char* dwScreenWidthAddr = (char*)D2MOO_Resolve("g_dwScreenWidth");
    if (!dwScreenWidthAddr) return 0;
    uint32_t g_dwScreenWidth = *(uint32_t*)dwScreenWidthAddr;
    
    char* bitMaskBase = (char*)D2MOO_Resolve("g_dwBitMaskTable");
    if (!bitMaskBase) return 0;
    
    int nCenterX = (int)(g_dwScreenWidth - 0x26C) / 2;
    
    int xLo = (int)((g_dwScreenWidth - nCenterX) + -0x1F);
    int xHi = (int)((g_dwScreenWidth - nCenterX) + -0x12);
    if (!((xLo <= (int)nMouseX) && ((int)nMouseX <= xHi))) {
        return 0;
    }
    
    nCenterX = (int)(*(uint32_t*)(bitMaskBase + 0x12 * 4u) - 0x1A4) / 2;
    
    if (!((nCenterX + 0x23 <= (int)nMouseY) && ((int)nMouseY <= nCenterX + 0x2F))) {
        return 0;
    }
    
    return 1;
}
