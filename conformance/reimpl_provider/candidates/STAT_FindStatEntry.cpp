// D2MOO_REIMPL_EXPORT: STAT_FindStatEntry
#include "../provider_runtime.h"

extern "C" uint32_t __stdcall STAT_FindStatEntry(void* pUnit, int nStatId, uint32_t dwFlagMask) {
    uint32_t pStatListAddr = *(uint32_t*)((uint8_t*)pUnit + 0x5C);

    if (pStatListAddr == 0) {
        return 0;
    }

    int nField10 = *(int*)((uint8_t*)pStatListAddr + 0x10);
    if (nField10 >= 0) {
        return 0;
    }

    uint32_t pEntry = *(uint32_t*)((uint8_t*)pStatListAddr + 0x3C);
    if ((dwFlagMask & 0x2000u) != 0) {
        pEntry = *(uint32_t*)((uint8_t*)pStatListAddr + 0x40);
        dwFlagMask &= 0xFFFFDFFFu;
    }

    while (pEntry != 0) {
        if (*(int*)((uint8_t*)pEntry + 0x14) == nStatId) {
            if (dwFlagMask == 0) {
                return pEntry;
            }
            if ((*(uint32_t*)((uint8_t*)pEntry + 0x10) & dwFlagMask) != 0) {
                return pEntry;
            }
        }
        pEntry = *(uint32_t*)((uint8_t*)pEntry + 0x2C);
    }

    return 0;
}
