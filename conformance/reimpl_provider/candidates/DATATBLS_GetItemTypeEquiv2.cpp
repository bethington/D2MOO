#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypeEquiv2

extern "C" int __stdcall DATATBLS_GetItemTypeEquiv2(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    // Check dwType == 4
    if (*(int*)((char*)pUnit + 0x0) != 4) return 0;

    // Call DATATBLS_GetItemTypeFromUnit via injected resolver (stdcall, 1 stack arg)
    typedef int (__stdcall *DATATBLS_GetItemTypeFromUnit_t)(void*);
    DATATBLS_GetItemTypeFromUnit_t _f =
        (DATATBLS_GetItemTypeFromUnit_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_f == nullptr) return 0;
    int iVar1 = _f(pUnit);

    // Result must be non-negative
    if (iVar1 < 0) return 0;

    // g_pDataTables is a pointer variable; deref once to get the struct base
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;
    char* base = (char*)*(void**)_g;

    // Bounds check: index < itemTypesTxtRecordCount (offset +0xBFC)
    int count = *(int*)(base + 0xbfc);
    if (iVar1 >= count) return 0;

    // Resolve ItemTypes base (offset +0xBF8) and compute pointer (stride 0xE4)
    int typesBase = *(int*)(base + 0xbf8);
    int ptr = iVar1 * 0xe4 + typesBase;
    if (ptr == 0) return 0;

    // Return sign-extended short at +0xE (MOVSX word -> i32)
    return *(short*)((char*)ptr + 0xe);
}
