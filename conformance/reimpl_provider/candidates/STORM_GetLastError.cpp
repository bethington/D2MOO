#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STORM_GetLastError
extern "C" uint32_t __stdcall STORM_GetLastError(void)
{
	// g_dwLastError_6fba4968 is a data DWORD (name starts g_dw, not g_p),
	// so STEP 1: base is the ADDRESS of the DWORD, no pointer deref needed.
	char* base = (char*)D2MOO_Resolve("g_dwLastError_6fba4968");
	if (!base)
		return 0xDEADBEEFu; // resolver not injected / name unknown -> obvious mismatch

	// STEP 2: the decompile is `return g_dwLastError_6fba4968;` -- a bare
	// reference to the DWORD value. Replacing `_g_dwLastError_6fba4968`
	// with `base` (the address) requires reading the DWORD at that address.
	return *(uint32_t*)base;
}
