#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypeField13_ByUnit

extern "C" uint32_t __stdcall DATATBLS_GetItemTypeField13_ByUnit(void* pUnit)
{
    if (pUnit == nullptr) return 0;
    if (*(uint32_t*)pUnit != 4) return 0;

    typedef int (__stdcall *DATATBLS_GetItemTypeFromUnit_t)(void*);
    DATATBLS_GetItemTypeFromUnit_t _f = (DATATBLS_GetItemTypeFromUnit_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_f == nullptr) return 0;
    int itemType = _f(pUnit);
    if (itemType < 0) return 0;

    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;
    char* base = *(char**)_g;

    uint32_t count = *(uint32_t*)(base + 0xbfc);
    if ((uint32_t)itemType >= count) return 0;

    char* tableBase = *(char**)(base + 0xbf8);
    char* entry = tableBase + (itemType * 0xe4);
    if (entry == nullptr) return 0;

    return (uint32_t)*(uint8_t*)(entry + 0x13);
}
