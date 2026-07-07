// datatable_rowcount.cpp -- STATEFUL rung 2: prove a function against REAL LOADED
// GAME DATA, with the reimpl reading the global BY NAME via the injected
// verified-address resolver (data-import resolver) -- NOT a hardcoded address.
//
// PD2's GetDataTableRowEntryCount (0x0a0b0, __stdcall(byte rowIndex)->int -- Ghidra
// mislabels it __fastcall; the disassembly MOVZX [ESP+4]/RET 0x4 is ground truth)
// scans the global g_dwDataTableBase array (600-byte stride per row) for the first
// zero entry and returns the count. The reimpl reads the SAME live global, resolved
// by name (g_dwDataTableBase -> verified address) through the resolver D2Debugger
// injects via D2MOO_Provider_Init (provider_runtime.h). This is the principled
// successor to the earlier hardcoded-0x6fdee2cc demo. Read-only immutable loaded
// data -> safe to call from the oracle thread.

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetDataTableRowEntryCount
extern "C" int __stdcall GetDataTableRowEntryCount(unsigned char bRowIndex)
{
	// &g_dwDataTableBase is the array base; resolve it by verified name.
	int* base = (int*)D2MOO_Resolve("g_dwDataTableBase");
	if (!base)
		return -0x7FFFFFFF; // resolver not injected / name unknown -> obvious mismatch

	int* p = (int*)((char*)base + (unsigned)bRowIndex * 600u);
	int count = 0, scanned = 0;
	while (true)
	{
		if (p[-1] == 0) return count;
		if (p[0]  == 0) return count + 1;
		if (p[1]  == 0) return count + 2;
		if (p[2]  == 0) return count + 3;
		if (p[3]  == 0) return count + 4;
		if (p[4]  == 0) return count + 5;
		scanned += 6; p += 6; count += 6;
		if (0x95 < scanned) return count;
	}
}
