#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypeCodeByIndex
// scalar nIndex -> ushort item-type code at g_pItemTypeCodeTable[nIndex], else 0xFFFFFFFF.
// Bounds vs g_dwItemRecordCount. (Original aborts if the table ptr is null -- unreachable live.)
// Strongly VARIED output (distinct type codes) -> a discriminating prove_spec target.
extern "C" unsigned int __stdcall DATATBLS_GetItemTypeCodeByIndex(int nIndex)
{
    void* _cnt = D2MOO_Resolve("g_dwItemRecordCount");
    if (_cnt == nullptr) return 0xFFFFFFFF;
    if (nIndex < 0 || nIndex >= *(int*)_cnt) return 0xFFFFFFFF;
    void* _g = D2MOO_Resolve("g_pItemTypeCodeTable");
    if (_g == nullptr) return 0xFFFFFFFF;
    unsigned short* table = *(unsigned short**)_g;
    if (table == nullptr) return 0xFFFFFFFF;
    return (unsigned int)table[nIndex];
}
