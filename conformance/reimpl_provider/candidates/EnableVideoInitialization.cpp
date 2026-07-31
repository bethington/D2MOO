#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: EnableVideoInitialization
extern "C" void __stdcall EnableVideoInitialization()
{
    // g_pSelectedEntity_6fbcc3b0 is a pointer variable (g_p prefix) -> deref once.
    void** ppSelectedEntity = (void**)D2MOO_Resolve("g_pSelectedEntity_6fbcc3b0");
    // &g_MISSILE_pDefaultAnimSeqData in decompile -> address of a data global.
    // Closest resolvable name in the provided list is g_pAnimSeqDataDefault.
    char* pAnimSeqData = (char*)D2MOO_Resolve("g_pAnimSeqDataDefault");

    if (!ppSelectedEntity || !pAnimSeqData)
        return; // resolver missing / name unknown -> obvious mismatch sentinel

    // g_pSelectedEntity_6fbcc3b0 = &g_MISSILE_pDefaultAnimSeqData;
    *ppSelectedEntity = pAnimSeqData;
}
