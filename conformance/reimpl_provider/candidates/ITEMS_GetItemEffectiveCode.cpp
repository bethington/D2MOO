#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: ITEMS_GetItemEffectiveCode

extern "C" uint8_t __stdcall ITEMS_GetItemEffectiveCode(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    /* CMP dword ptr [EAX],0x4 -- dwType check */
    if (*(int*)pUnit != 4) return 0;

    /* CALL DATATBLS_GetItemTypeFromUnit via resolver (stdcall, 1 stack arg) */
    typedef int (__stdcall *DATATBLS_GetItemTypeFromUnit_t)(void*);
    DATATBLS_GetItemTypeFromUnit_t _f =
        (DATATBLS_GetItemTypeFromUnit_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_f == nullptr) return 0;
    int nItemType = _f(pUnit);

    /* TEST EAX,EAX / JL -- signed-negative guard */
    if (nItemType < 0) return 0;

    /* g_pDataTables is a pointer-typed global: resolve -> deref once -> base */
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;
    char* base = (char*)*(void**)_g;

    /* CMP EAX,[ECX + 0xbfc] / JGE -- upper-bound check against count */
    if (nItemType >= *(int*)(base + 0xbfc)) return 0;

    /* IMUL EAX,EAX,0xe4 ; ADD EAX,[ECX + 0xbf8] -- record address */
    int record = nItemType * 0xe4 + *(int*)(base + 0xbf8);
    if (record == 0) return 0;

    /* MOVZX EAX, byte ptr [EAX + 0x12] -- low byte only */
    return *(uint8_t*)(record + 0x12);
}
