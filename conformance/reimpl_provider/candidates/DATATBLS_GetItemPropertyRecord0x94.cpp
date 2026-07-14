#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemPropertyRecord0x94

extern "C" int __stdcall DATATBLS_GetItemPropertyRecord0x94(void* pItem) {
    if (pItem == nullptr) return 0;

    // pItem->dwType (offset 0) == 4
    if (*(int*)pItem != 4) return -1;

    // pItem->pPlayerData (offset 0x14) != NULL
    void* pPlayerData = *(void**)((char*)pItem + 0x14);
    if (pPlayerData == nullptr) return -1;

    // *pPlayerData (offset 0) == 5  (COLLISION_ObjectCollisionData type 5 = special walkable object)
    if (*(int*)pPlayerData != 5) return -1;

    // Call GetItemProperty(pItem) -> index
    typedef int (__stdcall *GetItemProperty_t)(void*);
    GetItemProperty_t pGetItemProperty = (GetItemProperty_t)D2MOO_Resolve("GetItemProperty");
    if (pGetItemProperty == nullptr) return -1;
    int nItemPropertyIndex = pGetItemProperty(pItem);

    // index < 0 -> fail
    if (nItemPropertyIndex < 0) return -1;

    // Resolve g_pDataTables (pointer variable -> deref once to get base)
    void* g_pDataTables_addr = D2MOO_Resolve("g_pDataTables");
    if (g_pDataTables_addr == nullptr) return -1;
    char* base = (char*)*(void**)g_pDataTables_addr;

    // Bounds check: index < sgptDataTables[1].nItemPropertyTableCount (offset 0xc1c)
    int nCount = *(int*)(base + 0xc1c);
    if (nItemPropertyIndex >= nCount) return -1;

    // Compute record = index * 0x1B8 + sgptDataTables[1].nItemPropertyTableBase (offset 0xc18)
    uint32_t nTableBase = *(uint32_t*)(base + 0xc18);
    char* pRecord = (char*)((uint32_t)nItemPropertyIndex * 0x1b8u + nTableBase);
    if (pRecord == nullptr) return -1;

    // Return sign-extended short at record + 0x2E (MOVSX word ptr)
    return (int)*(short*)(pRecord + 0x2e);
}
