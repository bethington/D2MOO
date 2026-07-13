#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: STAT_FindStatEntry

extern "C" uint32_t __stdcall STAT_FindStatEntry(void* pUnit, int nStatId, uint32_t dwFlagMask)
{
    if (pUnit == nullptr) return 0;

    int* pStatList = *(int**)((char*)pUnit + 0x5C);
    if (pStatList == nullptr) return 0;

    int nField10 = *(int*)((char*)pStatList + 0x10);
    if (nField10 >= 0) return 0;          // JS: continue only when bit31 (sign) is set

    int* pEntry = *(int**)((char*)pStatList + 0x3C);

    if ((dwFlagMask & 0x2000) != 0) {     // TEST DH,0x20
        pEntry = *(int**)((char*)pStatList + 0x40);
        dwFlagMask = dwFlagMask & 0xFFFFDFFF;
    }

    while (pEntry != nullptr) {
        if (*(int*)((char*)pEntry + 0x14) == nStatId) {
            if (dwFlagMask == 0) {
                return (uint32_t)pEntry;
            }
            if ((*(uint32_t*)((char*)pEntry + 0x10) & dwFlagMask) != 0) {
                return (uint32_t)pEntry;
            }
        }
        pEntry = *(int**)((char*)pEntry + 0x2C);
    }

    return 0;
}
