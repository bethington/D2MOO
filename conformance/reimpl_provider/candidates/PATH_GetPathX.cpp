#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: PATH_GetPathX

extern "C" unsigned int __stdcall PATH_GetPathX(void* pPath)
{
    if (pPath == nullptr) return 0;
    return *(unsigned int*)((char*)pPath + 0xc);
}
