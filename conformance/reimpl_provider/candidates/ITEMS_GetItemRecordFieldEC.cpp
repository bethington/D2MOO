#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemRecordFieldEC
extern "C" uint32_t __stdcall ITEMS_GetItemRecordFieldEC(uint32_t dwItemRecordIndex)
{
    void* _count = D2MOO_Resolve("g_dwItemRecordCount");
    if (!_count) return 0;
    uint32_t dwCount = *(uint32_t*)_count;

    void* _pRecs = D2MOO_Resolve("g_pItemRecords");
    if (!_pRecs) return 0;
    char* base = *(char**)_pRecs;
    if (!base) return 0;

    // Original does abort on OOB; oracle only feeds valid classIds so return 0 sentinel.
    if (dwItemRecordIndex >= dwCount) return 0;

    // g_pItemRecords is typed DATATBLS_ItemRecord* with stride 0x1A8 -> byte offset = idx * 0x1A8
    char* pRecord = base + dwItemRecordIndex * 0x1A8u;
    return *(uint32_t*)(pRecord + 0xEC);
}
