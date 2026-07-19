#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypePropertyByte15

extern "C" uint32_t __stdcall DATATBLS_GetItemTypePropertyByte15(void* p)
{
    if (p == nullptr) return 0;

    typedef int (__stdcall *DATATBLS_GetItemTypeFromUnit_t)(uint32_t);
    DATATBLS_GetItemTypeFromUnit_t _getType =
        (DATATBLS_GetItemTypeFromUnit_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_getType == nullptr) return 0;

    int typeIndex = _getType((uint32_t)p);
    if (typeIndex < 0) return 0;

    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;
    char* base = *(char**)_g;
    if (base == nullptr) return 0;

    int count = *(int*)(base + 0xBFC);
    if (typeIndex >= count) return 0;

    char* pItemTypesTxt = *(char**)(base + 0xBF8);
    char* record = pItemTypesTxt + typeIndex * 0xE4;
    if (record == nullptr) return 0;

    return (uint32_t)(uint8_t)*(record + 0x15);
}
