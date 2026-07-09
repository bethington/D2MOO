#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetAnimSequenceRecord
extern "C" void* __stdcall GetAnimSequenceRecord(int nSequenceId)
{
    // g_dwObjectsTxtRecordCount: count value -> resolver returns &count, deref once.
    int* pCount = (int*)D2MOO_Resolve("g_dwObjectsTxtRecordCount");
    // g_pObjectsTxtTable: pointer variable -> resolver returns &ptr, deref once for the base.
    char* base = (char*)*(void**)D2MOO_Resolve("g_pObjectsTxtTable");

    if (!pCount || !base)
        return (void*)0xBAADF00D; // resolver missing sentinel

    if ((nSequenceId < *pCount) && (-1 < nSequenceId)) {
        return (void*)(base + nSequenceId * 0x1c0);
    }
    return (void*)0; // fatal branch (CleanupAndAbort/_exit) not compileable; oracle uses valid inputs
}
