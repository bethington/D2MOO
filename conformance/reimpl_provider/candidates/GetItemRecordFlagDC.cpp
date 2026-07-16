// D2MOO_REIMPL_EXPORT: GetItemRecordFlagDC
#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetItemRecordFlagDC
extern "C" uint32_t __stdcall GetItemRecordFlagDC(uint32_t nItemCode)
{
    void* cnt_addr = D2MOO_Resolve("g_dwItemRecordCount");
    void* ptr_addr = D2MOO_Resolve("g_pItemRecords");
    if (!cnt_addr || !ptr_addr) return 0u;

    uint32_t count = *(uint32_t*)cnt_addr;
    char* base = (char*)*(void**)ptr_addr;
    if (!base) return 0u;

    if (nItemCode < count) {
        uint32_t v = *(uint32_t*)(base + (size_t)nItemCode * 0x1A8u + 0xDCu);
        return v & 1u;
    }
    return 0u;
}
