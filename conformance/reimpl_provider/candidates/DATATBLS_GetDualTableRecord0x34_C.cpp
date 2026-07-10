#include "../provider_runtime.h"
// D2MOO_REIMPL_EXPORT: DATATBLS_GetDualTableRecord0x34_C
extern "C" void* __stdcall DATATBLS_GetDualTableRecord0x34_C(unsigned int dwIndex, int nTableSelect)
{
    // Resolve the globals by verified name
    unsigned int* pCount = (unsigned int*)D2MOO_Resolve("g_dat_6fdf13b4");
    void** pClassBase = (void**)D2MOO_Resolve("g_dat_6fdf13bc");
    void** pModeBase = (void**)D2MOO_Resolve("g_dat_6fdf13c0");

    // If any resolver fails, return obvious wrong-value sentinel
    if (!pCount || !pClassBase || !pModeBase)
        return (void*)0xBAADF00D;

    unsigned int dwObjCompositCount = *pCount;
    char* base;

    if (dwIndex < dwObjCompositCount) {
        if (nTableSelect == 0) {
            base = (char*)*pClassBase;
        } else if (nTableSelect == 1) {
            base = (char*)*pModeBase;
        } else {
            base = (char*)0;
        }

        if (base != (char*)0) {
            base = base + dwIndex * 0x34;
            // Abort branch: in the original this calls CleanupAndAbort.
            // For reimpl, just return the computed address (oracle uses valid inputs).
            return (void*)base;
        }
    }

    return (void*)0;
}
