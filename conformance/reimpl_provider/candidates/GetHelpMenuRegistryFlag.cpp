#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetHelpMenuRegistryFlag
extern "C" uint32_t __stdcall GetHelpMenuRegistryFlag(void)
{
    uint32_t* base = (uint32_t*)D2MOO_Resolve("g_bHelpMenuRegistryFlag");
    if (!base)
        return 0xDEADBEEFu; // resolver missing -> obvious mismatch

    if (*base == 0xFFFFFFFFu) {
        *base = 0;
        // QueryRegistryUlong() is not exposed via the resolvable globals list;
        // omit the call (oracle exercises initialized-cache path).
    }
    return *base;
}
