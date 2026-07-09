#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: PATH_GetOriginX
extern "C" unsigned int __stdcall PATH_GetOriginX(void* pPath)
{
    if (pPath == nullptr) return 0;
    return *(unsigned int*)pPath;
}
