#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SetLastNetworkError
// NEEDS GLOBAL: g_dwLastNetworkError
extern "C" void __fastcall SetLastNetworkError(uint32_t in_EAX)
{
	// &g_dwLastNetworkError is the storage; resolve it by verified name.
	// g_dwLastNetworkError is a plain data global (not a pointer variable).
	uint32_t* base = (uint32_t*)D2MOO_Resolve("g_dwLastNetworkError");
	if (!base)
		return; // resolver not injected / name unknown -> obvious mismatch (no-op)

	// g_dwLastNetworkError = in_EAX;
	*(uint32_t*)(base + 0) = in_EAX;
}
