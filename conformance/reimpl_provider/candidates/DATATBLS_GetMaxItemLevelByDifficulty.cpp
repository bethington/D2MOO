#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetMaxItemLevelByDifficulty
extern "C" int __stdcall DATATBLS_GetMaxItemLevelByDifficulty(int nDifficulty)
{
	// g_pExperienceTxtRecords is a POINTER VARIABLE (g_p*), so the decompile's
	// bare _g_pExperienceTxtRecords refers to its VALUE; resolve the address of
	// the pointer variable and deref ONCE to obtain the record base.
	void* resolved = D2MOO_Resolve("g_pExperienceTxtRecords");
	if (!resolved)
		return -1; // resolver not injected / name unknown -> obvious mismatch

	char* base = *(char**)resolved;
	if (!base)
		return -1; // record table not loaded -> obvious mismatch

	// Algorithm from the spec:
	//   if (-1 < nDifficulty && nDifficulty < 7)
	//     return (&record->dwAmazon)[nDifficulty];
	//   else
	//     return record->dwAmazon;
	// dwAmazon is the first field of the ExperienceTxt record (offset 0), and
	// the seven contiguous int fields (Amazon..Assassin) form an int array of
	// length 7 indexed by difficulty.
	if ((-1 < nDifficulty) && (nDifficulty < 7)) {
		return *(int*)((char*)base + nDifficulty * sizeof(int));
	}
	return *(int*)base;
}
