#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: MISSILE_GetMissileTypeShort04
extern "C" int __stdcall MISSILE_GetMissileTypeShort04(void* pUnit) {
    if (pUnit == nullptr) return 0;

    if (*(int*)((char*)pUnit + 0x0) != 3) return -1;

    void* pItemData = *(void**)((char*)pUnit + 0x14);
    if (pItemData == nullptr) return -1;

    return (int)*(short*)((char*)pItemData + 0x4);
}
