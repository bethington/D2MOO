#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypeClass

extern "C" int __stdcall DATATBLS_GetItemTypeClass(void* pUnit)
{
    typedef void* (__stdcall *DATATBLS_GetItemTypeFromUnit_t)(void*);

    if (pUnit == nullptr) return 0;
    if (*(int*)pUnit != 4) return 0;

    DATATBLS_GetItemTypeFromUnit_t _f =
        (DATATBLS_GetItemTypeFromUnit_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_f == nullptr) return 0;

    int iVar1 = (int)_f(pUnit);
    if (iVar1 < 0) return 0;

    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;
    char* base = *(char**)_g;

    int nItemTypesTxtRecordCount = *(int*)(base + 0xbfc);
    if (iVar1 >= nItemTypesTxtRecordCount) return 0;

    char* pItemTypesTxt = *(char**)(base + 0xbf8);
    char* record = pItemTypesTxt + iVar1 * 0xe4;
    if (record == nullptr) return 0;

    return *(uint8_t*)(record + 0x21) < 7;
}
