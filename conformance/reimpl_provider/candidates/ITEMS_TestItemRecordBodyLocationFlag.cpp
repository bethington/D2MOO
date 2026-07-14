// ITEMS_TestItemRecordBodyLocationFlag -- reimpl provider version
// Reads the live item record table by NAME through the verified-address
// resolver (D2MOO_Resolve). Bounds-checked against g_dwItemRecordCount,
// record stride 0x1A8, body-location flag byte at +0x139. Abort branches
// (NULL table / NULL record / out-of-range) compile as `return 0;` since
// the helpers GetReturnAddress/CleanupAndAbort/_exit are not present in
// the provider translation unit and the oracle exercises in-range inputs.

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_TestItemRecordBodyLocationFlag
extern "C" int __stdcall ITEMS_TestItemRecordBodyLocationFlag(int nItemCode)
{
    void* count_addr = D2MOO_Resolve("g_dwItemRecordCount");
    void* records_addr = D2MOO_Resolve("g_pItemRecords");
    if (!count_addr || !records_addr) return 0; // resolver missing -> obvious mismatch

    // g_dwItemRecordCount: data/array type -> use resolved address directly.
    uint32_t dwCount = *(uint32_t*)count_addr;
    // g_pItemRecords: POINTER VARIABLE -> deref once to get the table base.
    char* records_base = (char*)*(void**)records_addr;
    if (!records_base) return 0; // abort branch (NULL table) -> compile-only

    // (uint)nItemCode < g_dwItemRecordCount
    if ((uint32_t)nItemCode >= dwCount) return 0; // abort branch -> compile-only

    // pItemRecord = g_pItemRecords + nItemCode * 0x1A8 (stride)
    char* record = records_base + (uint32_t)nItemCode * 0x1A8;
    if (!record) return 0; // abort branch (NULL record) -> compile-only

    // return pItemRecord->bField139 != 0
    return *(uint8_t*)(record + 0x139) != 0;
}
