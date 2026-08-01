#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: IsTargetingEnabled
extern "C" int __stdcall IsTargetingEnabled(void)
{
	// &g_dwInteractionMode is a DWORD data variable (starts with g_dw, not g_p).
	// D2MOO_Resolve returns the address of the symbol -- use it directly as the base.
	int* base = (int*)D2MOO_Resolve("g_dwInteractionMode");
	if (!base)
		return 0xDEADBEEF; // resolver not injected / name unknown -> obvious mismatch
	return *base != 0;
}
