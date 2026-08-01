#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetLastError
extern "C" uint32_t __stdcall GetLastError(void)
{
    uint32_t* base = (uint32_t*)D2MOO_Resolve("g_nLastNpcMenuError");
    if (!base)
        return 0xDEADBEEFu; // resolver not injected / name unknown -> obvious mismatch
    return *base;
}
