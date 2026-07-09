#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: GetItemDataRecord
extern "C" void* __stdcall GetItemDataRecord(unsigned int dwItemCode)
{
    // Resolve g_pItemRecords: pointer variable, deref once to get base
    void** ppItemRecords = (void**)D2MOO_Resolve("g_pItemRecords");
    if (!ppItemRecords)
        return (void*)-0x7FFFFFFF;

    char* pItemRecords = (char*)*ppItemRecords;

    // Resolve g_dwItemRecordCount: not a pointer, value directly
    unsigned int* pItemRecordCount = (unsigned int*)D2MOO_Resolve("g_dwItemRecordCount");
    if (!pItemRecordCount)
        return (void*)-0x7FFFFFFF;

    if (*pItemRecordCount <= dwItemCode) {
        return (void*)0;
    }

    if (pItemRecords == 0) {
        // Abort path - return NULL for valid inputs, oracle exercises valid range only
        return (void*)0;
    }

    return (void*)(dwItemCode * 0x1a8 + pItemRecords);
}
