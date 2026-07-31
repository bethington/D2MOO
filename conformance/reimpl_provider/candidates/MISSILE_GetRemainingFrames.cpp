#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: MISSILE_GetRemainingFrames
extern "C" int __stdcall MISSILE_GetRemainingFrames(void* pMissileWrapper)
{
    if (pMissileWrapper == nullptr) return 0;
    if (*(int*)pMissileWrapper != 3) return 0;
    void* pMissileData = *(void**)((char*)pMissileWrapper + 0x14);
    if (pMissileData == nullptr) return 0;
    return (int)*(short*)((char*)pMissileData + 0x0E) - (int)*(short*)((char*)pMissileData + 0x10);
}
