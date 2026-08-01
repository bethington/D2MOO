#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SetErrorHandlingDisabled
extern "C" void __stdcall SetErrorHandlingDisabled(uint32_t dwDisable)
{
	// _g_dwErrorHandlingDisabled -> drop leading underscore -> "g_dwErrorHandlingDisabled"
	// Plain data uint global (no `g_p` prefix, no `T*` type) -> resolve returns the address directly.
	uint32_t* p = (uint32_t*)D2MOO_Resolve("g_dwErrorHandlingDisabled");
	if (!p)
		return; // resolver missing -> obvious mismatch on global-state comparison
	*p = dwDisable;
}
