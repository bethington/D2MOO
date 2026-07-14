#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STATS_TestStatFlag
extern "C" uint32_t __fastcall STATS_TestStatFlag(void* pUnit, uint32_t dwFlagIndex)
{
	// g_pdwStatBitMasks -> verified as g_dat_6fdd90b0 (data/array base, NOT a pointer variable)
	char* masks_base = (char*)D2MOO_Resolve("g_dat_6fdd90b0");
	if (masks_base == (char*)0)
		return 0xDEADBEEFu; // resolver missing -> obvious mismatch

	if (pUnit == (void*)0)
		return 0;

	// UnitAny+0x5C -> pStatList
	void* pStatList = *(void**)((char*)pUnit + 0x5C);
	if (pStatList == (void*)0)
		return 0;

	// D2StatListStrc+0x10 -> validity flag (negative == valid stat list signal)
	int validity = *(int*)((char*)pStatList + 0x10);
	if (validity >= 0)
		return 0;

	// D2StatListStrc+0x38 -> flag array base pointer
	char* flagArray = *(char**)((char*)pStatList + 0x38);

	// flagArray[dwFlagIndex >> 5]  (signed-cast shift, exactly as decompiled)
	uint32_t flagDword = *(uint32_t*)(flagArray + ((int)dwFlagIndex >> 5) * 4);

	// g_pdwStatBitMasks[dwFlagIndex & 0x1F]
	uint32_t bitMask = *(uint32_t*)(masks_base + (dwFlagIndex & 0x1Fu) * 4);

	return flagDword & bitMask;
}
