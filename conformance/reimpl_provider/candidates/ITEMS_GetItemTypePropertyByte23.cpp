#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemTypePropertyByte23
// pUnit(item) -> GetItemTypeFromUnit -> ItemTypes[idx]+0x23 (g_pDataTables+0xbf8 records / +0xbfc count)
typedef int (__stdcall* _gtype_t)(void*);
extern "C" unsigned int __stdcall ITEMS_GetItemTypePropertyByte23(void* pItem)
{
    if (pItem == nullptr || *(unsigned int*)pItem != 4) return 0;
    _gtype_t _gt = (_gtype_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_gt == nullptr) return 0;
    int typeIdx = _gt(pItem);
    if (typeIdx < 0) return 0;
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;
    char* dt = (char*)*(void**)_g;
    if (dt == nullptr) return 0;
    if (typeIdx >= *(int*)(dt + 0xbfc)) return 0;
    char* rec = (char*)*(void**)(dt + 0xbf8) + typeIdx * 0xe4;
    return (unsigned int)*(unsigned char*)(rec + 0x23);
}
