// D2MOO_REIMPL_EXPORT: PATH_GetDynamicX
#include "../provider_runtime.h"

// Fixed-offset getter: reads the dynamic-X dword at path+0xc. Ghidra types the
// param as bare `int` (not int*), which routed it to the static harness (can't
// test real memory) -> harness_failed. It is a live-pointer getter; proven via
// the handle path against a captured live object.
extern "C" unsigned int __stdcall PATH_GetDynamicX(void* pPath)
{
    if (pPath == nullptr) return 0;
    return *(unsigned int*)((char*)pPath + 0xc);
}
