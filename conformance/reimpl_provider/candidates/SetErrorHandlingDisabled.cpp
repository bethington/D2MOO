#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SetErrorHandlingDisabled
extern "C" void __stdcall SetErrorHandlingDisabled(uint32_t dwDisable)
{
	// NEEDS GLOBAL: g_dwErrorHandlingDisabled
	uint32_t* base = (uint32_t*)D2MOO_Resolve("g_dwErrorHandlingDisabled");
	if (!base)
		return;
	*base = dwDisable;
}
