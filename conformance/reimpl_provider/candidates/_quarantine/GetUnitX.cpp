#include"../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetUnitX
// Stub: this function in this build is a hardcoded zero return.
// Plate comment mentions pUnit but the decompile signature is `void GetUnitX(void)`,
// no arg reads, no global reads, just `return 0`.
//
// Even though the spec says "this function reads GLOBAL game state", the
// decompiled body proves it does not -- it is a stub. No D2MOO_Resolve call
// is needed; we just return 0.
extern "C" uint32_t __stdcall GetUnitX()
{
    return 0;
}
