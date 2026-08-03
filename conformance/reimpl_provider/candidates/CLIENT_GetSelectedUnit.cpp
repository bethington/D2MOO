#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_GetSelectedUnit
extern "C" void* __stdcall CLIENT_GetSelectedUnit()
{
	char* selAddr = (char*)D2MOO_Resolve("g_pSelectedUnit");
	if (!selAddr) return 0;
	char* expAddr = (char*)D2MOO_Resolve("g_bExpansionVersionCheck");
	if (!expAddr) return 0;
	char* bucketsAddr = (char*)D2MOO_Resolve("g_pNpcHashTableBucket1");
	if (!bucketsAddr) return 0;

	// g_pSelectedUnit: pointer variable -- deref ONCE to get UnitAny*
	void* sel = *(void**)selAddr;
	if (!sel) return 0;

	// g_bExpansionVersionCheck: data byte -- read directly
	uint8_t expVal = *(uint8_t*)expAddr;

	// g_pNpcHashTableBucket1: pointer variable -- deref ONCE to get array base
	char* buckets = (char*)*(void**)bucketsAddr;
	if (!buckets) return 0;

	// Bucket index: (expVal & 0x7f) + 0x7f, range [0x7f..0xFE]
	uint32_t bucketIdx = (expVal & 0x7f) + 0x7f;
	char* pUnit = (char*)(*(void**)((char*)buckets + bucketIdx * 4));

	while (pUnit != NULL) {
		// Compare unit GUID at offset 0x0C against the byte (zero-extended)
		if (*(uint32_t*)(pUnit + 0x0C) == (uint32_t)expVal) {
			// Verify unit type is 1 (Monster/NPC) at offset 0x00
			if (*(uint32_t*)(pUnit + 0x00) == 1) {
				return (void*)pUnit;
			}
			// Abort branch (CleanupAndAbort) -- provider has no such helper
			return 0;
		}
		// Follow hash chain via pNextHash at +0xE4
		pUnit = (char*)(*(void**)(pUnit + 0xE4));
	}

	return 0;
}
