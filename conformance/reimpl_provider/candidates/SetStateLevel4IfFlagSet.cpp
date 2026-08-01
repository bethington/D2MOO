#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SetStateLevel4IfFlagSet
extern "C" void __fastcall SetStateLevel4IfFlagSet(int *pStateCtx)
{
	// g_dwLastError_6fba17c8 is a data symbol (g_dw prefix), so resolve
	// returns the address of the dword directly (no extra deref).
	char* base = (char*)D2MOO_Resolve("g_dwLastError_6fba17c8");
	if (!base)
		return; // resolver missing -> obvious mismatch

	if (*(char *)((int)pStateCtx + 6) != '\0') {
		*(int *)base = 4;
	}
}
