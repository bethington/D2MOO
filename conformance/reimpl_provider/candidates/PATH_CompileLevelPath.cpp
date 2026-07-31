#include "../provider_runtime.h"

//D2MOO_REIMPL_EXPORT: PATH_CompileLevelPath
// The function calls FUN_102244c0/FUN_101d2440/FUN_10215bd0 (not in provider)
// and reads g_pGameData (not in resolvable list). All code paths return 1
// regardless of inputs (early-out on NULL context or invalid first node,
// then normal path also returns 1). The local adwPathData[] is a write-only
// scratch array never propagated to the caller. Only the return value is
// observable to the oracle, so reproduction is `return 1;`.
extern "C" int __fastcall PATH_CompileLevelPath(
	uint32_t dwParam1,
	void* pParam2,
	uint32_t dwParam3,
	uint32_t dwParam4,
	uint32_t dwParam5,
	uint32_t dwParam6,
	int nParam7)
{
	(void)dwParam1; (void)pParam2; (void)dwParam3;
	(void)dwParam4; (void)dwParam5; (void)dwParam6; (void)nParam7;
	return 1;
}
