// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemStatCostTxtRecord
#include "../provider_runtime.h"

extern "C" void* __fastcall DATATBLS_GetItemStatCostTxtRecord(int nStatIndex)
{
    // g_pDataTables is a pointer variable -> dereference once to get the struct base
    char* base = (char*)*(void**)D2MOO_Resolve("g_pDataTables");
    if (!base)
        return (void*)-1; // resolver missing sentinel

    if ((-1 < nStatIndex) && (nStatIndex < *(int*)(base + 0xBD4))) {
        return (void*)(nStatIndex * 0x144 + *(int*)(base + 0xBCC));
    }
    return (void*)0x0;
}
