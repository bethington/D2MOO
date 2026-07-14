#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypeEquiv1

extern "C" int __stdcall DATATBLS_GetItemTypeEquiv1(void* pUnit)
{
    if (pUnit == nullptr) return 0;

    /* CMP dword ptr [EAX],0x4 -- pUnit->dwType must be 4 (ITEM) */
    if (*(int*)pUnit != 4) return 0;

    /* PUSH EAX / CALL 0x6fd730f0 -- DATATBLS_GetItemTypeFromUnit(pUnit)
       Returns signed int typeIndex in EAX. stdcall (callee cleans). */
    typedef int (__stdcall *DATATBLS_GetItemTypeFromUnit_t)(void*);
    DATATBLS_GetItemTypeFromUnit_t _f =
        (DATATBLS_GetItemTypeFromUnit_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_f == nullptr) return 0;
    int typeIndex = _f(pUnit);

    /* TEST EAX,EAX / JL -- typeIndex must be >= 0 */
    if (typeIndex < 0) return 0;

    /* MOV ECX,[0x6fde9e1c] -- g_pDataTables is a POINTER variable; deref once */
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 0;
    char* pDT = (char*)*(void**)_g;
    if (pDT == nullptr) return 0;

    /* CMP EAX,[ECX+0xbfc] / JGE -- nItemTypesTxtRecordCount */
    int count = *(int*)(pDT + 0xbfc);
    if (typeIndex >= count) return 0;

    /* MOV EDX,[ECX+0xbf8] -- pItemTypesTxt base pointer */
    char* base = (char*)*(void**)(pDT + 0xbf8);
    if (base == nullptr) return 0;

    /* IMUL EAX,EAX,0xe4 / ADD EAX,EDX -- record = base + typeIndex*0xE4 */
    char* pRec = base + typeIndex * 0xE4;

    /* TEST EAX,EAX / JZ -- record pointer non-NULL */
    if (pRec == nullptr) return 0;

    /* MOVSX EAX, word ptr [EAX+0xc] -- sign-extend short at +0xC */
    return (int)*(short*)(pRec + 0xC);
}
