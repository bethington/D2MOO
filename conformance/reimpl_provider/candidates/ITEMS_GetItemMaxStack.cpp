#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_GetItemMaxStack
extern "C" uint32_t __stdcall ITEMS_GetItemMaxStack(uint32_t dwClassId)
{
	// Resolve g_dwItemRecordCount (data variable -- use directly)
	uint32_t* pCount = (uint32_t*)D2MOO_Resolve("g_dwItemRecordCount");
	if (!pCount) return 0u;

	// Resolve g_pItemRecords (POINTER variable -- deref once to get table base)
	void* pItemRecsAddr = D2MOO_Resolve("g_pItemRecords");
	if (!pItemRecsAddr) return 0u;
	char* pItemRecords = *(char**)pItemRecsAddr;
	if (!pItemRecords) return 0u;

	// Bounds + non-null checks (failure -> abort path, return 0 sentinel)
	if (dwClassId >= *pCount) return 0u;

	// Compute record address: g_pItemRecords + dwClassId * 0x1A8
	char* record = pItemRecords + (uint32_t)dwClassId * 0x1A8u;
	if (record == (char*)0) return 0u;

	// Return the uint at record+0xE8 (max stack size)
	return *(uint32_t*)(record + 0xE8);
}
