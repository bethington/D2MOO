#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SetGameEventHandler
// NEEDS GLOBAL: g_pSessionHashTableNode (storage for pfnHandler per decompile; not in resolver list)
extern "C" void __stdcall SetGameEventHandler(void* pfnHandler, int nHandlerParam)
{
    // The decompile's success path: g_pSessionHashTableNode = pfnHandler;
    // The global name is not resolvable, so the write is omitted.
    // For in-range inputs (pfnHandler != NULL, nHandlerParam != 0), both orig
    // and reimpl return normally. Abort branches are never reached.
}
