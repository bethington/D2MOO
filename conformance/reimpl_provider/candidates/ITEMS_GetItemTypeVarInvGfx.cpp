#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: ITEMS_GetItemTypeVarInvGfx
extern "C" uint8_t __stdcall ITEMS_GetItemTypeVarInvGfx(void* pUnit) {
    if (pUnit == nullptr) return 0;

    // dwType == 4 (item unit)
    if (*(uint32_t*)pUnit != 4u) return 0;

    // Resolve DATATBLS_GetItemTypeFromUnit (stdcall, 1 stack arg, returns int typeIdx)
    typedef int (__stdcall *DATATBLS_GetItemTypeFromUnit_t)(void*);
    DATATBLS_GetItemTypeFromUnit_t _fType =
        (DATATBLS_GetItemTypeFromUnit_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_fType == nullptr) return 0;

    int typeIdx = _fType(pUnit);

    // typeIdx must be >= 0 (no -1 error)
    if (typeIdx < 0) return 0;

    // Resolve g_pDataTables (pointer variable -> deref once to get table base)
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;
    char* dtBase = *(char**)_g;

    // Bounds: typeIdx < *(int*)(dtBase + 0xbfc)
    int count = *(int*)(dtBase + 0xbfc);
    if (typeIdx >= count) return 0;

    // Base: *(char**)(dtBase + 0xbf8)
    char* txtBase = *(char**)(dtBase + 0xbf8);

    // record = txtBase + typeIdx * 0xE4
    char* rec = txtBase + typeIdx * 0xe4;
    if (rec == nullptr) return 0;

    // return byte at record + 0x23 (nVarInvGfx)
    return *(uint8_t*)(rec + 0x23);
}
