#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetExperienceLevelForDifficulty
extern "C" uint32_t __stdcall DATATBLS_GetExperienceLevelForDifficulty(int nDifficulty, int nQualityTier)
{
	if ((nDifficulty < 0) || (6 < nDifficulty)) {
		nDifficulty = 0;
	}
	void* resolved = D2MOO_Resolve("g_pExperienceTxtRecords");
	if (!resolved) return 0xDEADBEEFu;
	char* base = *(char**)resolved;
	if (!base) return 0xDEADBEEFu;
	// Decompile: return (&g_pExperienceTxtRecords[nQualityTier + 1].dwAmazon)[nDifficulty];
	// sizeof(ExperienceTxtRec) = 32 bytes (8 uints), offsetof(dwAmazon) = 0.
	// Derived from plate comment's lookup formula: nDifficulty + 8 + nQualityTier*8 (in uint units).
	return *(uint32_t*)(base + (nQualityTier + 1) * 32 + nDifficulty * 4);
}
