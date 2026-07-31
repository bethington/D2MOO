#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: MISSILE_SetMissileSourceData

extern "C" uint32_t __stdcall MISSILE_SetMissileSourceData(void* pContext, void* pItemCode)
{
    if (pContext == nullptr) return 0;

    uint32_t dwContextType = *(uint32_t*)pContext;
    if (dwContextType != 3u) return (uint32_t)pContext;

    void* innerPtr = *(void**)((char*)pContext + 0x14);
    if (innerPtr == nullptr) return 0;

    if (pItemCode != nullptr) {
        *(uint32_t*)((char*)innerPtr + 0x20) = *(uint32_t*)pItemCode;
        *(uint32_t*)((char*)innerPtr + 0x24) = *(uint32_t*)((char*)pItemCode + 0xc);
        return (uint32_t)innerPtr;
    }

    *(uint32_t*)((char*)innerPtr + 0x20) = 0;
    *(uint32_t*)((char*)innerPtr + 0x24) = 0xFFFFFFFFu;
    return (uint32_t)innerPtr;
}
