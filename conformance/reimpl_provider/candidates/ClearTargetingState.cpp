// D2MOO_REIMPL_EXPORT: ClearTargetingState
#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ClearTargetingState
extern "C" void __stdcall ClearTargetingState()
{
    int* dwTE = (int*)D2MOO_Resolve("g_dwTargetingEnabled");
    if (dwTE) *dwTE = 0;

    int* dwLE = (int*)D2MOO_Resolve("g_dwLastError_6fbcc3e0");
    if (dwLE) *dwLE = 0;

    char** pp1 = (char**)D2MOO_Resolve("g_pPlayerName_6fbcc3f8");
    if (pp1) *pp1 = (char*)0;

    char** pp2 = (char**)D2MOO_Resolve("g_pPlayerName_6fbcc3fc");
    if (pp2) *pp2 = (char*)0;
}
