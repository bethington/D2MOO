#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_GetRosterEntryNamePointer
extern "C" void* __fastcall CLIENT_GetRosterEntryNamePointer(uint32_t dwUnitGuid)
{
    // g_pRosterPetList is a pointer variable (starts with g_p) -- deref the resolved address ONCE.
    void* listHead = (void*)*(void**)D2MOO_Resolve("g_pRosterPetList");
    if (!listHead) return (void*)0xDEADBEEF; // resolver missing / symbol unknown -> obvious mismatch

    void* p = listHead;
    while (true) {
        if (p == 0) return (void*)0;
        if (*(uint32_t*)((char*)p + 0x08) == dwUnitGuid) break;
        p = *(void**)((char*)p + 0x30);
    }
    return (void*)((char*)p + 0x24);
}
