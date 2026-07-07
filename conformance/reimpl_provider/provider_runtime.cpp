// provider_runtime.cpp -- injected resolver storage (see provider_runtime.h).
#include "provider_runtime.h"

namespace {
	typedef void* (__cdecl* ResolveFn)(const char*);
	ResolveFn g_resolve = nullptr;
}

extern "C" {

// D2MOO_REIMPL_EXPORT: D2MOO_Provider_Init
// Injected by D2Debugger after MemoryLoadLibraryEx, passing D2MOO_ResolveGameFn.
void __cdecl D2MOO_Provider_Init(void* resolveGameFn)
{
	g_resolve = (ResolveFn)resolveGameFn;
}

// Verified NAME -> live game address. Null if unresolved / not injected.
void* __cdecl D2MOO_Resolve(const char* name)
{
	return g_resolve ? g_resolve(name) : nullptr;
}

}
