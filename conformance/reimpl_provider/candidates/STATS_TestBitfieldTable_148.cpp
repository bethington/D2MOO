#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STATS_TestBitfieldTable_148
extern "C" uint32_t __stdcall STATS_TestBitfieldTable_148(uint32_t dwStatId)
{
	// Resolve g_pDataTables (pointer variable -> deref once to get struct base)
	void* pDT_var = D2MOO_Resolve("g_pDataTables");
	if (!pDT_var) return 0xDEADBEEFu;
	char* pDataTables = *(char**)pDT_var;
	if (!pDataTables) return 0xDEADBEEFu;

	// Resolve g_dat_6fdd90b0 (= g_pdwStatBitMasks: 32-entry uint bit-mask table)
	int* pdwStatBitMasks = (int*)D2MOO_Resolve("g_dat_6fdd90b0");
	if (!pdwStatBitMasks) return 0xDEADBEEFu;

	// Validation per decompile:
	//   if (-1 < (int)dwStatId && (int)dwStatId < (int)g_pDataTables->dwBitCount)
	// dwBitCount at offset 0x144 (uint32, sits immediately before pStatBitfield @ 0x148)
	int signedStatId = (int)dwStatId;
	int dwBitCount = *(int*)(pDataTables + 0x144);

	if ((-1 < signedStatId) && (signedStatId < dwBitCount)) {
		// Load bitfield pointer at offset 0x148, then AND word[(dwStatId>>5)] with mask[(dwStatId & 0x1f)]
		int* pBitfield = *(int**)(pDataTables + 0x148);
		uint32_t word = (uint32_t)pBitfield[signedStatId >> 5];
		uint32_t mask = (uint32_t)pdwStatBitMasks[dwStatId & 0x1f];
		return word & mask;
	}
	return 0;
}
