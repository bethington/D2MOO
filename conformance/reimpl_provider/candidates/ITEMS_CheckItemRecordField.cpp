#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_CheckItemRecordField
extern "C" int __stdcall ITEMS_CheckItemRecordField(uint32_t dwItemRecordId)
{
    // Resolve g_dwItemRecordCount (data variable, not a pointer)
    uint32_t* pCount = (uint32_t*)D2MOO_Resolve("g_dwItemRecordCount");
    if (!pCount) return 0;

    // Resolve g_pItemRecords (pointer variable - deref once)
    void* pItemRecordsAddr = D2MOO_Resolve("g_pItemRecords");
    if (!pItemRecordsAddr) return 0;
    char* g_pItemRecords = *(char**)pItemRecordsAddr;
    if (g_pItemRecords == 0) return 0;

    // Resolve g_pDataTables (pointer variable - deref once)
    void* pDataTablesAddr = D2MOO_Resolve("g_pDataTables");
    if (!pDataTablesAddr) return 0;
    char* g_pDataTables = *(char**)pDataTablesAddr;
    if (g_pDataTables == 0) return 0;

    // Abort branch: return 0 instead of calling _exit (oracle stays in-range)
    if (*pCount <= dwItemRecordId) return 0;

    // Compute pItemRecordEntry = dwItemRecordId * 0x1A8 + g_pItemRecords
    char* pItemRecordEntry = g_pItemRecords + dwItemRecordId * 0x1A8;
    if (pItemRecordEntry == 0) return 0;

    // Read wField11E (short) at offset 0x11E
    short wField11E = *(short*)(pItemRecordEntry + 0x11E);
    int iVar1 = (int)wField11E;

    // Get DataTables fields
    char* pItemTypesTxt = *(char**)(g_pDataTables + 0x20);
    int nItemTypesTxtRecordCount = *(int*)(g_pDataTables + 0x24);

    // Check -1 < iVar1 (signed) and iVar1 < nItemTypesTxtRecordCount
    if (iVar1 < 0) return 0;
    if (iVar1 >= nItemTypesTxtRecordCount) return 0;

    // Compute iVar1 = iVar1 * 0xE4 + pItemTypesTxt
    iVar1 = iVar1 * 0xE4 + (int)pItemTypesTxt;
    if (iVar1 == 0) return 0;

    // Return *(byte *)(iVar1 + 0x21) < 7
    uint8_t bField21 = *(uint8_t*)((char*)iVar1 + 0x21);
    return bField21 < 7 ? 1 : 0;
}
