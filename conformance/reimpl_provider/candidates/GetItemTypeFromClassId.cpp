#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetItemTypeFromClassId
extern "C" int __stdcall GetItemTypeFromClassId(uint32_t dwClassId)
{
	void* pCount = D2MOO_Resolve("g_dwItemRecordCount");
	if (!pCount) return 0;
	uint32_t dwCount = *(uint32_t*)pCount;

	char* base = (char*)*(void**)D2MOO_Resolve("g_pItemRecords");
	if (!base) return 0;

	if (dwClassId >= dwCount) return 0;

	char* pRecord = base + dwClassId * 0x1A8u;
	if (!pRecord) return 0;

	short wType = *(short*)(pRecord + 0x11E);
	return (int)wType;
}
