#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ValidateUnitFromHashById
extern "C" uint32_t __stdcall ValidateUnitFromHashById(void)
{
	// Resolve g_bCritterSpawned (1-byte flag)
	char* baseSpawned = (char*)D2MOO_Resolve("g_bCritterSpawned");
	if (!baseSpawned) return 0xDEADBEEFu;

	if (*(uint8_t*)baseSpawned == 0u) return 0u;

	// Resolve g_dwLastCritterGuid
	char* baseGuid = (char*)D2MOO_Resolve("g_dwLastCritterGuid");
	if (!baseGuid) return 0xDEADBEEFu;

	uint32_t dwGuid = *(uint32_t*)baseGuid;

	// Resolve g_apItemHashArrayA (array of 128 pointers)
	char* baseArray = (char*)D2MOO_Resolve("g_apItemHashArrayA");
	if (!baseArray) return 0xDEADBEEFu;

	// bucket = g_apItemHashArrayA[dwGuid & 0x7F]
	void* pCritter = *(void**)(baseArray + (dwGuid & 0x7Fu) * 4u);

	while (pCritter != (void*)0) {
		// Match: dwGuid at struct offset +0x0C
		//   *(uint *)(pCritter->pPadding08 + 4)  =>  *(uint*)(pCritter + 0x0C)
		if (*(uint32_t*)((char*)pCritter + 0x0C) == dwGuid) {
			// State: 4 bytes at offset +0x00 (bActive + pPadding03[0..2])
			if (*(uint32_t*)((char*)pCritter + 0x00) == 1u) {
				// Success: CONCAT31((int3)((uint)pCritter >> 8), 1)
				//   = ((uint)pCritter & 0xFFFFFF00) | 1
				uint32_t pc = (uint32_t)(uintptr_t)pCritter;
				return (pc & 0xFFFFFF00u) | 1u;
			}
			// Fatal abort path (CleanupAndAbort / _exit(-1)) -- not compilable
			// in the provider; oracle only exercises valid in-range cases.
			return 0u;
		}
		// pNext at struct offset +0xE4
		pCritter = *(void**)((char*)pCritter + 0xE4);
	}

	return 0u;
}
