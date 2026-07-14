#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetMonsterLevelTilePropBit7

extern "C" int __stdcall DATATBLS_GetMonsterLevelTilePropBit7(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    // dwType at offset 0x0 must == 1 (Monster)
    if (*(uint32_t*)pUnit != 1u) return 0;

    // dwTxtFileNo (monster classId) at offset 0x4 (signed range check)
    int classId = *(int*)((char*)pUnit + 0x4);
    if (classId < 0) return 0;

    // g_pDataTables -- pointer variable; resolve then deref once.
    void* pDataTablesVar = D2MOO_Resolve("g_pDataTables");
    if (pDataTablesVar == nullptr) return 0;
    char* pDataTables = (char*)*(void**)pDataTablesVar;

    // nMonStatsCount at +0xa80 -- upper-bound check.
    int count = *(int*)(pDataTables + 0xa80);
    if (classId >= count) return 0;

    // pMonStatsTxt base at +0xa78; entry stride is 0x1a8.
    char* pTableBase = *(char**)(pDataTables + 0xa78);
    char* pEntry = pTableBase + classId * 0x1a8;
    if (pEntry == nullptr) return 0;

    // bPropFlags byte at +0xc (zero-extended to 32-bit by MOVZX).
    uint32_t flags = (uint32_t)*(uint8_t*)(pEntry + 0xc);

    // g_dat_6fdd90b0 -- pointer to bit-mask table; deref once.
    void* pBitMasksVar = D2MOO_Resolve("g_dat_6fdd90b0");
    if (pBitMasksVar == nullptr) return 0;
    char* pBitMasks = (char*)*(void**)pBitMasksVar;

    // gdwBitMasks[7] mask at +0x1c.
    uint32_t mask = *(uint32_t*)(pBitMasks + 0x1c);

    return (int)(flags & mask);
}
