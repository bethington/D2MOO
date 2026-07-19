#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemRecordFieldE4
extern "C" uint32_t __stdcall ITEMS_GetItemRecordFieldE4(uint32_t dwItemRecordIndex)
{
	// Resolve both globals by name (verified addresses injected by the oracle).
	void* countAddr  = D2MOO_Resolve("g_dwItemRecordCount");
	void* recordsAddr = D2MOO_Resolve("g_pItemRecords");

	// Resolver not injected / name unknown -> obvious mismatch sentinel.
	if (!countAddr || !recordsAddr)
		return 0xDEADBEEFu;

	// g_dwItemRecordCount is a scalar (g_dw*), read its value directly.
	uint32_t dwRecordCount = *(uint32_t*)countAddr;

	// g_pItemRecords is a POINTER VARIABLE (g_p*) -- the decompile's
	// bare _g_pItemRecords is the POINTER's VALUE, so deref resolved addr ONCE.
	char* base = (char*)*(void**)recordsAddr;
	if (!base)
		return 0xDEADBEEFu;

	// Bounds check: index < count, then index the records array
	// (stride 0x1A8 per ITEMS_ItemRecord) and read uint32 at offset 0xE4.
	if (dwItemRecordIndex < dwRecordCount)
	{
		return *(uint32_t*)(base + (uint32_t)dwItemRecordIndex * 0x1A8u + 0xE4u);
	}

	// Abort branch -- never reached under oracle (valid in-range inputs only).
	return 0u;
}
