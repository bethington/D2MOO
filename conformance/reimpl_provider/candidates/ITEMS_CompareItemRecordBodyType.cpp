#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ITEMS_CompareItemRecordBodyType
extern "C" int __stdcall ITEMS_CompareItemRecordBodyType(uint32_t dwClassId, int nBodyType)
{
	// g_dwItemRecordCount: data variable -- resolve address of the variable itself.
	uint32_t* dwItemRecordCount = (uint32_t*)D2MOO_Resolve("g_dwItemRecordCount");
	if (!dwItemRecordCount) return 0;

	// g_pItemRecords: pointer variable -- resolve address of the variable, then deref ONCE for the array base.
	void* pItemRecordsVar = D2MOO_Resolve("g_pItemRecords");
	if (!pItemRecordsVar) return 0;
	char* base = (char*)*(void**)pItemRecordsVar;
	if (!base) return 0;

	// dwClassId < g_dwItemRecordCount  (out-of-range abort branch -> return 0 to compile)
	if (dwClassId >= *dwItemRecordCount) return 0;

	// g_pItemRecords + dwClassId*0x1A8  (record pointer arithmetic from the decompile)
	char* record = base + dwClassId * 0x1A8u;
	if (!record) return 0;

	// dwBodyType lives at offset +0xC0 (int)
	return (*(int*)(record + 0xC0) == nBodyType) ? 1 : 0;
}
