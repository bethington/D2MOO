#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemStorePage

extern "C" uint8_t __stdcall DATATBLS_GetItemStorePage(void* pUnit) {
    if (pUnit == nullptr) return 7;

    // pUnit->dwType == 4 (offset 0)
    if (*(uint32_t*)pUnit != 4) return 7;

    // EAX = DATATBLS_GetItemTypeFromUnit(pUnit)
    typedef int (__stdcall *DATATBLS_GetItemTypeFromUnit_t)(void*);
    DATATBLS_GetItemTypeFromUnit_t _f =
        (DATATBLS_GetItemTypeFromUnit_t)D2MOO_Resolve("DATATBLS_GetItemTypeFromUnit");
    if (_f == nullptr) return 7;
    int idx = _f(pUnit);

    // TEST EAX,EAX / JL: signed check idx < 0
    if (idx < 0) return 7;

    // ECX = *g_pDataTables (deref the pointer variable)
    void* _g = D2MOO_Resolve("g_pDataTables");
    if (_g == nullptr) return 7;
    char* dt = *(char**)_g;

    // CMP EAX,[ECX+0xbfc] / JGE: bounds check vs nItemTypesTxtRecordCount
    if (idx >= *(int*)(dt + 0xbfc)) return 7;

    // EDX = [ECX+0xbf8] = g_pDataTables->pItemTypesTxt (table base)
    char* pItemTypesTxt = *(char**)(dt + 0xbf8);

    // IMUL EAX,EAX,0xe4; ADD EAX,EDX  =>  rec = idx*0xE4 + base
    char* pRec = pItemTypesTxt + (uint32_t)idx * 0xE4u;
    if (pRec == nullptr) return 7;

    // MOV AL,[EAX+0x21]; CMP AL,7; MOVZX EAX,AL; JC -> return AL else fall through to 7
    uint8_t bStorePage = *(uint8_t*)(pRec + 0x21);
    if (bStorePage < 7) return bStorePage;
    return 7;
}
