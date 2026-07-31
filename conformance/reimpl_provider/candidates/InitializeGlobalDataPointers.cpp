#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitializeGlobalDataPointers
//
// NOTE: this function writes the live game globals `g_ppDataTbl1..7` from
// the live pointer globals `g_pDataTbl1..7`; the proof therefore compares
// the POST-CALL global state of the running game. None of the required
// symbols are in the resolver list yet, so this reimpl is marked:
//   NEEDS GLOBAL: g_pDataTbl1
//   NEEDS GLOBAL: g_ppDataTbl1
//   NEEDS GLOBAL: g_pDataTbl2
//   NEEDS GLOBAL: g_ppDataTbl2
//   NEEDS GLOBAL: g_pDataTbl3
//   NEEDS GLOBAL: g_ppDataTbl3
//   NEEDS GLOBAL: g_pDataTbl4
//   NEEDS GLOBAL: g_ppDataTbl4
//   NEEDS GLOBAL: g_pDataTbl5
//   NEEDS GLOBAL: g_ppDataTbl5
//   NEEDS GLOBAL: g_pDataTbl6
//   NEEDS GLOBAL: g_ppDataTbl6
//   NEEDS GLOBAL: g_pDataTbl7
//   NEEDS GLOBAL: g_ppDataTbl7
// (skipped until added to the resolver allow-list)

extern "C" void __stdcall InitializeGlobalDataPointers(void)
{
	// Pairs (odd N): copy the POINTER VALUE of g_pDataTblN
	//                (g_p- prefix => deref the resolved slot ONCE)
	//                into the slot pointed to by g_ppDataTblN.
	// Pairs (even N): copy the ADDRESS of g_pDataTblN (no deref -- the
	//                 resolved slot address itself) into g_ppDataTblN.

	void* a1 = D2MOO_Resolve("g_pDataTbl1");
	void* b1 = D2MOO_Resolve("g_ppDataTbl1");
	void* a2 = D2MOO_Resolve("g_pDataTbl2");
	void* b2 = D2MOO_Resolve("g_ppDataTbl2");
	void* a3 = D2MOO_Resolve("g_pDataTbl3");
	void* b3 = D2MOO_Resolve("g_ppDataTbl3");
	void* a4 = D2MOO_Resolve("g_pDataTbl4");
	void* b4 = D2MOO_Resolve("g_ppDataTbl4");
	void* a5 = D2MOO_Resolve("g_pDataTbl5");
	void* b5 = D2MOO_Resolve("g_ppDataTbl5");
	void* a6 = D2MOO_Resolve("g_pDataTbl6");
	void* b6 = D2MOO_Resolve("g_ppDataTbl6");
	void* a7 = D2MOO_Resolve("g_pDataTbl7");
	void* b7 = D2MOO_Resolve("g_ppDataTbl7");

	// Bail on any missing symbol so the post-call state diverges from the
	// original (loud misconfig, never a silent match).
	if (!a1 || !b1 || !a2 || !b2 || !a3 || !b3 || !a4 || !b4 ||
		!a5 || !b5 || !a6 || !b6 || !a7 || !b7)
		return;

	*(void**)b1 = *(void**)a1; // value  -> g_ppDataTbl1
	*(void**)b2 = a2;          // addr   -> g_ppDataTbl2
	*(void**)b3 = *(void**)a3; // value  -> g_ppDataTbl3
	*(void**)b4 = a4;          // addr   -> g_ppDataTbl4
	*(void**)b5 = *(void**)a5; // value  -> g_ppDataTbl5
	*(void**)b6 = a6;          // addr   -> g_ppDataTbl6
	*(void**)b7 = *(void**)a7; // value  -> g_ppDataTbl7
}
