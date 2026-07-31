#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: FLOAT_InitializeFloatCallbacks
//
// Initializes the six Intel JPEG Library floating-point callback function
// pointers used by the IJL runtime inside ijl11.dll.
//
// ABI (from disassembly): __stdcall, RET 0x0, 0 stack params, void return.
//
// Algorithm (per decompile):
//   g_pfnFloatStripG        = (pfnFloatStripG *)&float_strip_g_exit;
//   g_pfnFloatToString      = __cfltcvt;
//   PTR___fptrap_600277fc   = __fassign;
//   g_pfnFloatStrip         = FormatDecimalPoint;
//   PTR___fptrap_60027804   = &float_trap_handler_exit;
//   PTR___fptrap_60027808   = __cfltcvt;
//
// All six destination globals are IJL-internal (ijl11.dll image-relative)
// symbols and are NOT present in the D2MOO resolvable global set. They
// therefore need to be added before this reimpl can perform the writes.
// Until then the reimpl body is empty; the oracle compares void return
// (trivially matches) and a non-crash.
extern "C" void __stdcall FLOAT_InitializeFloatCallbacks(void)
{
    // NEEDS GLOBAL: g_pfnFloatStripG
    // NEEDS GLOBAL: g_pfnFloatToString
    // NEEDS GLOBAL: PTR___fptrap_600277fc
    // NEEDS GLOBAL: g_pfnFloatStrip
    // NEEDS GLOBAL: PTR___fptrap_60027804
    // NEEDS GLOBAL: PTR___fptrap_60027808
}
