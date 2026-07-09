#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: PATH_GetCollisionField
extern "C" __declspec(dllexport) int __stdcall PATH_GetCollisionField(void* pUnit)
{
    if (pUnit == nullptr) return 0;
    void* pPathNode = *(void**)((char*)pUnit + 0x2C);
    return *(int*)((char*)pPathNode + 0x48);
}
