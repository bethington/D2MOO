#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypeDataByte

extern "C" uint32_t __stdcall DATATBLS_GetItemTypeDataByte(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    /* dwType == 4 (Item type) */
    if (*(int*)pUnit != 4) return 7;

    /* Call DATATBLS_GetItemTypeFromUnit(pUnit) -- stdcall, 1 ptr arg */
    typedef int (__stdcall *GetItemTypeFromUnit_t)(void*);
    GetItemTypeFromUnit_t _f = (GetItemTypeFromUnit_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_f == nullptr) return 7;
    int itemType = _f(pUnit);

    /* signed check: must be >= 0 */
    if (itemType < 0) return 7;

    /* Resolve g_pDataTables (pointer variable -> deref once to table base) */
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 7;
    char* dataTables = *(char**)_g;

    /* itemType < nItemTypesTxtRecordCount (count at +0xbfc) */
    int recordCount = *(int*)(dataTables + 0xbfc);
    if (itemType >= recordCount) return 7;

    /* pItemTypesTxt base at +0xbf8 */
    char* pItemTypesTxt = *(char**)(dataTables + 0xbf8);

    /* entry = pItemTypesTxt + itemType * 0xE4 (entry stride 228) */
    char* entry = pItemTypesTxt + itemType * 0xe4;
    if (entry == nullptr) return 7;

    /* MOVZX EAX, byte ptr [entry + 0x1F] -- return zero-extended byte */
    return (uint32_t)*(uint8_t*)(entry + 0x1f);
}
