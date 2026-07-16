#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetDataTableRowEntryCount
extern "C" int __stdcall DATATBLS_GetDataTableRowEntryCount(uint8_t bRowIndex)
{
	// &g_dwDataTableBase is the array base; resolve it by verified name.
	int* base = (int*)D2MOO_Resolve("g_dwDataTableBase");
	if (!base)
		return -0x7FFFFFFF; // resolver not injected / name unknown -> obvious mismatch

	// Each row is 600 bytes wide; pEntrySlot = &g_dwDataTableBase + bRowIndex * 0x96 (DWORD stride 4 -> 600).
	int* p = (int*)((char*)base + (unsigned)bRowIndex * 600u);
	int count = 0, scanned = 0;
	while (true)
	{
		if (p[-1] == 0) return count;        // pEntrySlot[-1].pSlot5 terminator
		if (p[0]  == 0) return count + 1;    // pSlot0
		if (p[1]  == 0) return count + 2;    // pSlot1
		if (p[2]  == 0) return count + 3;    // pSlot2
		if (p[3]  == 0) return count + 4;    // pSlot3
		if (p[4]  == 0) return count + 5;    // pSlot4
		scanned += 6; p += 6; count += 6;
		if (0x95 < scanned) return count;
	}
}
