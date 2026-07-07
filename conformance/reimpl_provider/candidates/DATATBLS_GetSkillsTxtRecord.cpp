// D2MOO_REIMPL_EXPORT: DATATBLS_GetSkillsTxtRecord
// Get Skills.txt record pointer by skill ID with bounds validation.
// - Returns NULL for negative or out-of-range indices.
// - Resolves g_pDataTables by name to get the live global base.
// - Computes record pointer as: pSkillsTxt + (index * 0x23C).
#include "../provider_runtime.h"

extern "C" void* __fastcall DATATBLS_GetSkillsTxtRecord(int nSkillId)
{
    // g_pDataTables is a pointer variable (name starts with g_p), so deref once.
    char* base = (char*)*(void**)D2MOO_Resolve("g_pDataTables");
    if (!base)
        return (void*)0x0;

    int nSkillsCount = *(int*)(base + 0xBA0);
    if (nSkillId < 0 || nSkillId >= nSkillsCount)
        return (void*)0x0;

    return (void*)(*(int*)(base + 0xB98) + nSkillId * 0x23C);
}
