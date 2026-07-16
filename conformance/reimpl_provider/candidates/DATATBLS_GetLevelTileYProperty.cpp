#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetLevelTileYProperty
extern "C" int __stdcall DATATBLS_GetLevelTileYProperty(int nLevelTileIndex, int nUnitLevel)
{
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;

    // g_pDataTables is a POINTER variable -- deref resolved address ONCE to get the table base.
    char* base = (char*)*(void**)_g;
    if (base == nullptr) return 0;

    // Bounds: nLevelTileIndex must satisfy 0 <= nLevelTileIndex < nLevelTileYEntryCount.
    int nLevelTileYEntryCount = *(int*)(base + 0xba0);
    if (nLevelTileIndex < 0 || nLevelTileIndex >= nLevelTileYEntryCount) {
        return 0;
    }

    // pEntries = *(int*)(base + 0xb98);  pEntry = pEntries + nLevelTileIndex * 0x23c
    int pEntriesAddr = *(int*)(base + 0xb98);
    char* pEntry = (char*)pEntriesAddr + nLevelTileIndex * 0x23c;
    if (pEntry == nullptr) {
        return 0;
    }

    // Entry fields at fixed offsets (per spec structure layout).
    short  nField18C  = *(short*)(pEntry + 0x18C);   // signed level multiplier
    short  nField18A  = *(short*)(pEntry + 0x18A);   // signed addend
    uint8_t bShiftBits = *(uint8_t*)(pEntry + 0x188);  // shift mask

    // (((int)nField18C * (nUnitLevel - 1) + (int)nField18A) << (bShiftBits & 0x1f)) >> 8
    int nPropertyValue = (((int)nField18C * (nUnitLevel - 1) + (int)nField18A) << (bShiftBits & 0x1f)) >> 8;
    if (nPropertyValue < 0) {
        nPropertyValue = 0;
    }
    return nPropertyValue;
}
