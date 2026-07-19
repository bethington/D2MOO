#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypeBodyLoc1

extern "C" uint8_t __stdcall DATATBLS_GetItemTypeBodyLoc1(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    /* pUnit->dwType at offset 0: must be 4 (item) */
    if (*(uint32_t*)pUnit != 4u) return 0;

    /* Resolve & call DATATBLS_GetItemTypeFromUnit (stdcall, 1 arg = pUnit pointer) */
    typedef int (__stdcall *DATATBLS_GetItemTypeFromUnit_t)(void*);
    DATATBLS_GetItemTypeFromUnit_t _f =
        (DATATBLS_GetItemTypeFromUnit_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_f == nullptr) return 0;
    int idx = _f(pUnit);

    /* Signed sign-check on returned index */
    if (idx < 0) return 0;

    /* g_pDataTables is a pointer variable: resolve to its address, then deref once */
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;
    char* dt = (char*)*(void**)_g;

    /* nItemTypesTxtRecordCount at D2DataTablesStrc +0xBFC -- signed compare via JGE */
    int count = *(int*)(dt + 0xBFC);
    if (idx >= count) return 0;

    /* pItemTypesTxt base at D2DataTablesStrc +0xBF8 (no null check at this load per disasm) */
    void* baseArray = *(void**)(dt + 0xBF8);

    /* record = baseArray + idx * 0xE4 ; then null-check the resulting record pointer */
    char* record = (char*)baseArray + idx * 0xE4;
    if (record == nullptr) return 0;

    /* D2ItemTypesTxt.nBodyLoc1 at +0x10 (byte, MOVZX zero-extends) */
    return *(uint8_t*)(record + 0x10);
}
