#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetBodyLocationProperty
// NOTE: g_abBodyLocationProperty is not in the resolvable globals list.
// Using g_dat_6fdef0f0 as the base; offset assumed to be 0.
// If this is wrong, the property is at a non-zero offset within that structure.
extern "C" unsigned char __stdcall DATATBLS_GetBodyLocationProperty(unsigned char bBodyLoc)
{
    // g_abBodyLocationProperty is a data array (name starts with g_, not g_p_)
    // Use resolved address directly as the base pointer.
    void* _g = D2MOO_Resolve("g_dat_6fdef0f0");
    if (!_g)
        return 0; // resolver not injected -> obvious mismatch
    
    unsigned char* base = (unsigned char*)_g;
    
    // Translate decompile literally: (&g_abBodyLocationProperty)[bBodyLoc]
    return base[bBodyLoc];
}
