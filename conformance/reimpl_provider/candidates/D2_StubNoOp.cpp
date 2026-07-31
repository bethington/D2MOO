// D2MOO_REIMPL_EXPORT: StubNoOp
#include "../provider_runtime.h"

extern "C" void __stdcall StubNoOp(void)
{
    // No-op: stub function that immediately returns.
    // No global state is read; no work is performed.
    return;
}
