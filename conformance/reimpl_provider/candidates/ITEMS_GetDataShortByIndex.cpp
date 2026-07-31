#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetDataShortByIndex
extern "C" uint16_t __stdcall ITEMS_GetDataShortByIndex(void* pUnit, int nSuffixIndex) {
    if (pUnit == nullptr) return 0;

    uint32_t dwType = *(uint32_t*)((char*)pUnit + 0x00);
    if (dwType != 4) return 0;

    void* pItemData = *(void**)((char*)pUnit + 0x14);
    if (pItemData == nullptr) return 0;

    // Magic suffix array: 3 prefix entries at +0x38 (3 * 2 = 6 bytes), suffixes begin at +0x3e.
    // Read uint16 at (pItemData + 0x3e + nSuffixIndex * 2).
    uint16_t result = *(uint16_t*)((char*)pItemData + 0x3e + nSuffixIndex * 2);
    return result;
}
