#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: BinkSetSimulate
extern "C" void __stdcall BinkSetSimulate(uint32_t dwSimMode)
{
    // NEEDS GLOBAL: g_dwBinkSimMode
    int* base = (int*)D2MOO_Resolve("g_dwBinkSimMode");
    if (!base)
        return;
    *base = (int)dwSimMode;
}
