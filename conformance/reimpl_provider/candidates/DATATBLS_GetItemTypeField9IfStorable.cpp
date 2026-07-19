#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypeField9IfStorable

extern "C" uint8_t __stdcall DATATBLS_GetItemTypeField9IfStorable(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    int* pUnit_i = (int*)pUnit;
    if (*pUnit_i != 4) return 0;

    // Local storage bytes for x and z dimensions (storage[1] unused by caller logic).
    uint8_t storage[3] = {0, 0, 0};

    typedef int (__stdcall *Dims_t)(void*, uint8_t*, uint8_t*);
    Dims_t _dims = (Dims_t)D2MOO_Resolve("DATATBLS_GetItemTypeStorageDimensions");
    if (_dims == nullptr) return 0;
    _dims(pUnit, &storage[0], &storage[2]);

    if (storage[0] == 0 && storage[2] == 0) return 0;

    typedef int (__stdcall *GetType_t)(void*);
    GetType_t _getType = (GetType_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_getType == nullptr) return 0;
    int nItemType = _getType(pUnit);

    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;
    char* base = (char*)*(void**)_g;
    if (base == nullptr) return 0;

    int count = *(int*)(base + 0xbfc);
    if (nItemType < 0 || nItemType >= count) return 0;

    int offset = *(int*)(base + 0xbf8) + nItemType * 0xe4;
    if (offset == 0) return 0;

    return *(uint8_t*)(offset + 9);
}
