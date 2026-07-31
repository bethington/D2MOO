#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: IsPointInScrollBarThumbRegion
extern "C" int __fastcall IsPointInScrollBarThumbRegion(int edx_unused, int nThumbX, int nThumbY)
{
    // g_dwScreenWidth: DWORD data (4 bytes, unsigned)
    char* pScreenWidth = (char*)D2MOO_Resolve("g_dwScreenWidth");
    if (!pScreenWidth) return 0;
    
    // g_nScrollThumbYPos: int data (4 bytes, signed)
    char* pScrollThumbYPos = (char*)D2MOO_Resolve("g_nScrollThumbYPos");
    if (!pScrollThumbYPos) return 0;
    
    // g_pdwScrollIndicatorCount: pointer to int (deref once for the int value)
    char* pScrollIndicatorCountPtr = (char*)D2MOO_Resolve("g_pdwScrollIndicatorCount");
    if (!pScrollIndicatorCountPtr) return 0;
    int nScrollIndicatorCount = *(int*)pScrollIndicatorCountPtr;
    
    // dwThumbXBoundary = (int)(g_dwScreenWidth - 0x26c) / 2;
    uint32_t dwScreenWidth = *(uint32_t*)pScreenWidth;
    uint32_t dwThumbXBoundary = (int)(dwScreenWidth - 0x26c) / 2;
    
    // (int)((g_dwScreenWidth - dwThumbXBoundary) + -0x1f)
    int leftEdge = (int)(dwScreenWidth - dwThumbXBoundary) + (-0x1f);
    int rightEdge = (int)(dwScreenWidth - dwThumbXBoundary) + (-0x12);
    
    // _g_nScrollThumbYPos
    int nScrollThumbYPos = *(int*)pScrollThumbYPos;
    
    // _g_nScrollThumbYPos + (int)g_pdwScrollIndicatorCount * 0xc
    int thumbBottom = nScrollThumbYPos + nScrollIndicatorCount * 0xc;
    
    // Check all bounds
    if (leftEdge <= nThumbX && nThumbX <= rightEdge &&
        nScrollThumbYPos <= nThumbY && nThumbY <= thumbBottom)
        return 1;
    return 0;
}
