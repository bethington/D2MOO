#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: BinkBufferGetError
extern "C" void* __stdcall BinkBufferGetError(void)
{
    void* base = D2MOO_Resolve("g_szErrorBuffer");
    if (!base)
        return (void*)0xDEADBEEF;
    return base;
}
