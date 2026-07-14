#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemRecordFlags
extern "C" uint32_t __stdcall ITEMS_GetItemRecordFlags(uint32_t dwItemIndex)
{
    void* pCountAddr = D2MOO_Resolve("g_dwItemRecordCount");
    if (pCountAddr == nullptr) return 0;
    uint32_t dwItemRecordCount = *(uint32_t*)pCountAddr;

    void* pRecordsAddr = D2MOO_Resolve("g_pItemRecords");
    if (pRecordsAddr == nullptr) return 0;
    char* base = (char*)*(void**)pRecordsAddr;

    if (((dwItemIndex < dwItemRecordCount) && (base != nullptr)) &&
        ((base + (size_t)dwItemIndex * 0x1A8u) != nullptr)) {
        return *(uint32_t*)(base + (size_t)dwItemIndex * 0x1A8u + 0xDC) & 2u;
    }

    return 0;
}
