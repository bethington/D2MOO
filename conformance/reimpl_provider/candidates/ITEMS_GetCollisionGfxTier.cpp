#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: ITEMS_GetCollisionGfxTier

extern "C" uint8_t __stdcall ITEMS_GetCollisionGfxTier(void* pUnit)
{
    // Oracle never passes null -- just compile-safe stub.
    if (pUnit == nullptr) return 0;

    // pUnit->dwType (offset 0x00) must equal 4 (ITEM)
    if (*(uint32_t*)((char*)pUnit + 0x0) != 4u) return 0;

    // ---- DATATBLS_GetItemTypeFromUnit(pUnit) ----
    typedef int (__stdcall *DATATBLS_GetItemTypeFromUnit_t)(void*);
    DATATBLS_GetItemTypeFromUnit_t _getItemType =
        (DATATBLS_GetItemTypeFromUnit_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_getItemType == nullptr) return 0;
    int nItemTypeIdSigned = _getItemType(pUnit);

    // TEST EAX,EAX / JL -> signed < 0 returns 0
    if (nItemTypeIdSigned < 0) return 0;
    uint32_t dwItemTypeId = (uint32_t)nItemTypeIdSigned;

    // ---- g_pDataTables (resolve once, deref once) ----
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;
    char* dataTables = (char*)*(void**)_g;

    // CMP EAX,[ECX+0xbfc] / JGE  -> dwItemTypeId >= nItemTypesTxtRecordCount
    int nItemTypesTxtRecordCount = *(int*)(dataTables + 0xbfc);
    if (nItemTypeIdSigned >= nItemTypesTxtRecordCount) return 0;

    // pItemTypesTxt base (offset 0xbf8)
    void* pItemTypesTxtBase = *(void**)(dataTables + 0xbf8);

    // entry = dwItemTypeId * 0xE4 + pItemTypesTxt
    char* pItemTypeEntry = (char*)pItemTypesTxtBase + dwItemTypeId * 0xe4u;

    // Disasm JNZ at 6fd74647 -> entry pointer must be non-null
    if (pItemTypeEntry == nullptr) return 0;

    // ---- GetItemRecordGfxField(pUnit->dwTxtFileNo) ----
    typedef uint32_t (__stdcall *GetItemRecordGfxField_t)(uint32_t);
    GetItemRecordGfxField_t _getRecGfx =
        (GetItemRecordGfxField_t)D2MOO_Resolve("GetItemRecordGfxField");
    if (_getRecGfx == nullptr) return 0;

    // pUnit->dwTxtFileNo at offset 0x04
    uint32_t dwTxtFileNo = *(uint32_t*)((char*)pUnit + 0x4);
    uint32_t dwItemRecordGfxRaw = _getRecGfx(dwTxtFileNo);
    uint32_t dwResult = dwItemRecordGfxRaw & 0xffu;

    // ---- RefreshUnitCollisionSize(pUnit) ----
    typedef int (__stdcall *RefreshUnitCollisionSize_t)(void*);
    RefreshUnitCollisionSize_t _refreshSize =
        (RefreshUnitCollisionSize_t)D2MOO_Resolve("RefreshUnitCollisionSize");
    if (_refreshSize == nullptr) return 0;
    int nCollisionSize = _refreshSize(pUnit);

    // Select tier by collision size (signed thresholds from CMP/JG).
    // size <= 25 (CMP 0x19/JG): small   -> byte at entry + 0x1A
    // size 26..40 (CMP 0x28/JG): medium -> byte at entry + 0x1B
    // size >= 41:                       large  -> byte at entry + 0x1C
    uint32_t dwGfxTier;
    if (nCollisionSize < 0x1a) {
        dwGfxTier = (uint32_t)*(uint8_t*)(pItemTypeEntry + 0x1a);
    } else if (nCollisionSize < 0x29) {
        dwGfxTier = (uint32_t)*(uint8_t*)(pItemTypeEntry + 0x1b);
    } else {
        dwGfxTier = (uint32_t)*(uint8_t*)(pItemTypeEntry + 0x1c);
    }

    // CMP EBX,EAX / JL; MOV EBX,EAX -> result = min(gfxRecord, gfxTier)
    if (dwGfxTier <= dwResult) {
        dwResult = dwGfxTier;
    }

    // Disasm ends with MOV AL,BL -- only AL (low byte) is meaningful.
    return (uint8_t)(dwResult & 0xffu);
}
