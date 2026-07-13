#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetBodyLocPropertyByte
extern "C" uint8_t __stdcall DATATBLS_GetBodyLocPropertyByte(uint8_t bBodyLoc)
{
    // g_abBodyLocPropertyTbl resolves at runtime to g_dat_6fdef0a8; data/array base.
    char* base = (char*)D2MOO_Resolve("g_dat_6fdef0a8");
    if (base == (char*)0)
        return (uint8_t)0xFF; // sentinel: resolver missing -> obvious mismatch

    // (&g_abBodyLocPropertyTbl)[bBodyLoc]  -- direct byte-array indexing.
    return ((uint8_t*)base)[bBodyLoc];
}
