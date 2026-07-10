#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: MONSTER_GetBossCategory

extern "C" int __stdcall MONSTER_GetBossCategory(void* pUnit)
{
    if (pUnit == nullptr) {
        return 0;
    }
    
    int dwType = *(int*)((char*)pUnit + 0x0);
    if (dwType != 1) {
        return 0;
    }
    
    unsigned int dwClassId = *(unsigned int*)((char*)pUnit + 0x4);
    
    if (dwClassId <= 0x167) {
        if (dwClassId == 0x167) {
            return 3;
        }
        if (dwClassId == 0x10f) {
            return 1;
        }
        if (dwClassId == 0x152) {
            return 2;
        }
    }
    else if ((dwClassId >= 0x230) && (dwClassId <= 0x231)) {
        return 4;
    }
    
    return 0;
}
