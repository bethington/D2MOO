// datatbls_getmonstatstxtrecord.cpp -- D2MOO reimpl provider.
// Reads sgptDataTables (g_pDataTables) by verified-name resolve so it tracks
// whatever the live game has loaded. Algorithm matches the Ghidra decompile:
//   if (-1 < nMonClassId && nMonClassId < nMonStatsCount at +0xBC8)
//       return pMonStatsTxt at +0xBC4 + nMonClassId * 0xC4;
//   else return NULL.

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetMonStatsTxtRecord
extern "C" void* __fastcall DATATBLS_GetMonStatsTxtRecord(int nMonClassId)
{
	// Resolve sgptDataTables (g_pDataTables) by verified name.
	void* _g = D2MOO_Resolve("g_pDataTables");
	if (_g == nullptr)
		return (void*)0xDEADBEEF; // resolver missing -> obvious mismatch sentinel

	// g_pDataTables is a POINTER variable (sgptDataTables*); deref once to
	// get the struct base the decompile's g_pDataTables->... walks.
	char* base = *(char**)_g;
	if (base == nullptr)
		return (void*)0xDEADBEEF;

	// *(int*)&g_pDataTables->field_0xbc8 -- table row count
	int nMonStatsCount = *(int*)(base + 0xBC8);

	// Bounds check: TEST EAX,EAX + JL (negative rejected) + CMP/CMPL against count.
	if ((-1 < nMonClassId) && (nMonClassId < nMonStatsCount)) {
		// &g_pDataTables->pMonStatsTxt->bField00 + nMonClassId * 0xc4
		// bField00 is at offset 0 of the record, so this is just the base
		// pointer of the record array plus the 0xC4-strided element.
		char* pMonStatsTxt = *(char**)(base + 0xBC4);
		return (void*)(pMonStatsTxt + nMonClassId * 0xC4);
	}
	return (void*)0;
}
