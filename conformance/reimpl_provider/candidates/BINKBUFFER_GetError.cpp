#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: BINKBUFFER_GetError
extern "C" void* __stdcall BINKBUFFER_GetError(void)
{
	// Decompile: return (char *)&g_dwItemRecordCount;
	// g_dwItemRecordCount is a data global (g_dw* prefix, not g_p*), so the
	// resolved address IS the base we want to return.
	void* base = D2MOO_Resolve("g_dwItemRecordCount");
	if (!base)
		return (void*)0xDEADBEEF; // resolver missing / name unknown -> loud mismatch
	return base;
}
