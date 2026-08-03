#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetLastError
extern "C" uint32_t __stdcall GetLastError(void)
{
	// _g_dwLastError_6fbc9725 in Ghidra -> resolve as "g_dwLastError_6fbc9725".
	// Plain data global (no leading g_p*), use the resolver return directly as the base.
	uint32_t* base = (uint32_t*)D2MOO_Resolve("g_dwLastError_6fbc9725");
	if (!base)
		return 0xDEADBEEFu; // resolver missing / name unknown -> obvious mismatch sentinel

	// Decompile: return _g_dwLastError_6fbc9725;  -> deref the resolved address once.
	return *base;
}
