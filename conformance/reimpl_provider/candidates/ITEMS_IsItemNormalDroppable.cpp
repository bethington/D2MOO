#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_IsItemNormalDroppable
extern "C" int __stdcall ITEMS_IsItemNormalDroppable(uint32_t dwClassId)
{
	// g_dwItemRecordCount is a scalar DWORD (data, not pointer) -> resolve returns &var, read via offset 0.
	char* countBase = (char*)D2MOO_Resolve("g_dwItemRecordCount");
	if (!countBase) return 0;

	// g_pItemRecords is a POINTER VARIABLE (table base) -> deref the resolved address once to get the records array base.
	char* recordsBase = (char*)*(void**)D2MOO_Resolve("g_pItemRecords");
	if (!recordsBase) return 0;

	uint32_t dwItemRecordCount = *(uint32_t*)countBase;

	if (dwClassId < dwItemRecordCount)
	{
		// stride is 0x1a8 per ITEMS_ItemRecordEntry
		char* pRecord = recordsBase + dwClassId * 0x1a8u;

		uint8_t  bField12A = *(uint8_t *)(pRecord + 0x12A);
		uint16_t wField11E = *(uint16_t *)(pRecord + 0x11E);

		if ((bField12A == 0) && (wField11E != 0x27))
			return 1;
		return 0;
	}

	// abort path (validation failure): original calls CleanupAndAbort/_exit which are not
	// available in the provider -- return 0 so the oracle's in-range probe still compiles.
	return 0;
}
