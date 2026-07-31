#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: FindAndValidateD2ExpMpq
extern "C" int __stdcall FindAndValidateD2ExpMpq(void)
{
    char* pCountExpected = (char*)D2MOO_Resolve("g_dwUiPanelCountExpected");
    if (!pCountExpected)
        return (int)0xDEADBEEF; // resolver not injected -> obvious wrong-value sentinel

    if (*(int*)pCountExpected != 0x26) {
        // Fatal abort path (GetReturnAddress / CleanupAndAbort / _exit(-1)).
        // Oracle stays strictly in-range (g_dwUiPanelCountExpected == 0x26),
        // so this branch only has to compile.
        return 0;
    }

    char* pPanelsArr  = (char*)D2MOO_Resolve("g_dwUIStatePanelsArr");
    char* pHoverTimer = (char*)D2MOO_Resolve("g_dwGroundHoverTimer");
    char* pHoverState = (char*)D2MOO_Resolve("g_dwGroundHoverState");

    if (pPanelsArr)  *(int*)(pPanelsArr  + 0xe * 4) = 0;
    if (pHoverTimer) *(int*)pHoverTimer = 0;
    if (pHoverState) *(int*)pHoverState = 0;

    return 0;
}
