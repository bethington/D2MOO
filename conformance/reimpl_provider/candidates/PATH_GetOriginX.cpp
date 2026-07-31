#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: PATH_GetOriginX
extern "C" uint32_t __stdcall PATH_GetOriginX(void* pPath)
{
    if (pPath == nullptr) return 0;

    uint16_t xOffset = *(uint16_t*)((char*)pPath + 0x00);
    uint16_t xPos    = *(uint16_t*)((char*)pPath + 0x02);

    return ((uint32_t)xPos << 16) | (uint32_t)xOffset;
}
