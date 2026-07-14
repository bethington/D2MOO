#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetMonsterMaxComponentVisualTier

extern "C" uint8_t __stdcall DATATBLS_GetMonsterMaxComponentVisualTier(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    int type = *(int*)pUnit;
    if (type != 4) return 0;

    typedef int (__stdcall *DATATBLS_GetItemTypeFromUnit_t)(void*);
    DATATBLS_GetItemTypeFromUnit_t _f =
        (DATATBLS_GetItemTypeFromUnit_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_f == nullptr) return 0;

    int idx = _f(pUnit);
    if (idx < 0) return 0;

    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;

    char* dt = *(char**)_g;
    int count = *(int*)(dt + 0xbfc);
    if (idx >= count) return 0;

    char* base = *(char**)(dt + 0xbf8);
    char* entry = base + idx * 0xe4;
    if (entry == nullptr) return 0;

    return *(uint8_t*)(entry + 0x11);
}
