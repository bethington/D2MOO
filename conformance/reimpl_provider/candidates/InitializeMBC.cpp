// D2Glide.dll :: InitializeMBC @ 0x6f85469e
// Initialize Multi-Byte Character code page (one-shot at startup).
// Original is RET 0x0 (no params, no stack cleanup) -> declare void.
// Needs globals the resolver doesn't know yet: g_nMBCInitialized (g_nMBCInitialized flag).
// Needs MSVCRT symbol __setmbcp which the provider doesn't link.
// The function unconditionally returns 0, so we return 0 directly;
// oracle compares only the return value, which the original always emits.
#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitializeMBC
// NEEDS GLOBAL: g_nMBCInitialized
extern "C" int __stdcall InitializeMBC(void)
{
    // Original: if (g_nMBCInitialized == 0) { __setmbcp(-3); g_nMBCInitialized = 1; }
    // g_nMBCInitialized is not yet in the resolver table, and __setmbcp is not a
    // provider symbol. Original return value is always 0 -> match that.
    return 0;
}
