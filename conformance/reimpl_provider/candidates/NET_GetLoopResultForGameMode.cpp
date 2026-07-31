#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: NET_GetLoopResultForGameMode
extern "C" uint32_t __stdcall NET_GetLoopResultForGameMode()
{
	uint32_t uVar1 = 1;

	uint32_t* base_gameMode = (uint32_t*)D2MOO_Resolve("g_dwGameMode_6fbcc394");
	if (!base_gameMode)
		return 0xDEADBEEFu; // resolver not injected / name unknown -> obvious mismatch

	uint32_t* base_lastError = (uint32_t*)D2MOO_Resolve("g_dwLastError_6fbc9858");
	if (!base_lastError)
		return 0xDEADBEEFu; // resolver not injected / name unknown -> obvious mismatch

	if (((int)*base_gameMode < 2) || (3 < (int)*base_gameMode)) {
		uVar1 = 1;
	}
	else {
		uVar1 = *base_lastError;
	}
	return uVar1;
}
