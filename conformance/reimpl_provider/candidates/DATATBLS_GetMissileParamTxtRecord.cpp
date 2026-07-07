#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetMissileParamTxtRecord
extern "C" void* __fastcall DATATBLS_GetMissileParamTxtRecord(int nMissileParamId)
{
    // g_pDataTables is a pointer variable (starts with g_p), deref once
    void** pDataTablesPtr = (void**)D2MOO_Resolve("g_pDataTables");
    if (!pDataTablesPtr)
        return (void*)0xBAADF00D; // resolver missing sentinel

    char* pDataTables = (char*)*pDataTablesPtr;
    if (!pDataTables)
        return (void*)0xBAADF00D; // null data tables sentinel

    int nMissileParamEntryCount = *(int*)(pDataTables + 0xb6c);

    if ((-1 < nMissileParamId) && (nMissileParamId < nMissileParamEntryCount)) {
        int pMissileParamEntries = *(int*)(pDataTables + 0xb64);
        return (void*)(nMissileParamId * 0x1a4 + pMissileParamEntries);
    }
    return (void*)0x0;
}
