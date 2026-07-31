#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetCursorItem
extern "C" uint32_t __stdcall GetCursorItem()
{
	// g_dwItemEventResult is a data DWORD (name prefix g_dw, not a pointer variable).
	// D2MOO_Resolve returns the ADDRESS of the symbol; deref it once to get the DWORD value.
	char* base = (char*)D2MOO_Resolve("g_dwItemEventResult");
	if (!base)
		return 0xDEADBEEFu; // resolver missing -> obvious mismatch sentinel
	// Bare reference _g_dwItemEventResult in the decompile = the DWORD value at that address.
	return *(uint32_t*)base;
}
