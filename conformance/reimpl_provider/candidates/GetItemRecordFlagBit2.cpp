#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetItemRecordFlagBit2
extern "C" uint32_t __stdcall GetItemRecordFlagBit2(int nClassId)
{
	// g_pItemRecords is a pointer variable (g_p*) -> dereference resolved address ONCE
	void* pRecPtrAddr = D2MOO_Resolve("g_pItemRecords");
	if (!pRecPtrAddr)
		return 0xDEADBEEFu;
	char* pRecords = *(char**)pRecPtrAddr;
	if (!pRecords)
		return 0xDEADBEEFu;

	// g_dwItemRecordCount is a data variable (g_dw*) -> use resolved address directly
	uint32_t* pCount = (uint32_t*)D2MOO_Resolve("g_dwItemRecordCount");
	if (!pCount)
		return 0xDEADBEEFu;

	// Bounds check (abort branch must not fire under oracle -- return 0 instead)
	if ((uint32_t)nClassId >= *pCount)
		return 0;

	// Read uint at record + 0xDC (stride 0x1A8), isolate bit 2 (mask 4)
	uint32_t val = *(uint32_t*)(pRecords + (uint32_t)nClassId * 0x1A8u + 0xDCu);
	return val & 4u;
}
