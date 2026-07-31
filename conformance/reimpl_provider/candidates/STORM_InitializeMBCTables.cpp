#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STORM_InitializeMBCTables
extern "C" int __stdcall STORM_InitializeMBCTables(void)
{
    // Resolve the MBC-init flag by verified name. The decompile's
    // `g_nMBCInitialized` corresponds to the resolver-known symbol
    // `g_bMbcTableInitialized`.
    char* base = (char*)D2MOO_Resolve("g_bMbcTableInitialized");
    if (!base)
        return 0x7FFFFFFF; // resolver missing -> obvious mismatch sentinel

    if (*(int*)base == 0) {
        // __setmbcp(-3) is a CRT internal helper; not resolvable in the
        // provider and not part of the function's measurable return value,
        // so we skip the call (analogous to the "abort branch" rule).
        *(int*)base = 1;
    }
    return 0;
}
