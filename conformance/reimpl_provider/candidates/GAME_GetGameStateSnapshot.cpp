#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GAME_GetGameStateSnapshot
// NEEDS GLOBAL: g_pGameStateVtbl
// NEEDS GLOBAL: g_pCurrentSNetProvider
// NEEDS GLOBAL: g_pCritSec
extern "C" int __stdcall GAME_GetGameStateSnapshot(void* pStateBuffer)
{
	// Validate pStateBuffer is non-NULL and magic == 0x24
	if (!pStateBuffer || *(int*)pStateBuffer != 0x24)
	{
		return 0;
	}

	// Resolve globals via injected resolver
	// g_pGameStateVtbl, g_pCurrentSNetProvider are pointer variables (g_p*) -> deref once
	// g_pCritSec is a CRITICAL_SECTION struct base -> use directly
	char* pVtbl = (char*)D2MOO_Resolve("g_pGameStateVtbl");
	if (pVtbl) pVtbl = *(char**)pVtbl;

	char* pProvider = (char*)D2MOO_Resolve("g_pCurrentSNetProvider");
	if (pProvider) pProvider = *(char**)pProvider;

	void* pCritSec = D2MOO_Resolve("g_pCritSec");

	// Enter critical section
	void* pEnterFn = D2MOO_Resolve("g_pfnEnterCritSec");
	if (pEnterFn && pCritSec)
	{
		((void(*)(void*))pEnterFn)(pCritSec);
	}

	// Clear destination buffer fields at offsets +0x04 through +0x20
	// (the decompile shows many redundant stores; net effect clears 7 dwords)
	*(int*)((char*)pStateBuffer + 0x04) = 0;
	*(int*)((char*)pStateBuffer + 0x08) = 0;
	*(int*)((char*)pStateBuffer + 0x0c) = 0;
	*(int*)((char*)pStateBuffer + 0x10) = 0;
	*(int*)((char*)pStateBuffer + 0x14) = 0;
	*(int*)((char*)pStateBuffer + 0x18) = 0;
	*(int*)((char*)pStateBuffer + 0x1c) = 0;
	*(int*)((char*)pStateBuffer + 0x20) = 0;

	// Check game state initialization
	if (pVtbl != 0 && pProvider != 0)
	{
		// Copy 9 dwords (36 bytes) from g_pCurrentSNetProvider + 0x214 to destination
		char* pSource = pProvider + 0x214;
		int* pDest = (int*)pStateBuffer;
		for (int i = 0; i < 9; i++)
		{
			pDest[i] = *(int*)(pSource + i * 4);
		}

		// Leave critical section
		void* pLeaveFn = D2MOO_Resolve("g_pfnLeaveCritSec");
		if (pLeaveFn && pCritSec)
		{
			((void(*)(void*))pLeaveFn)(pCritSec);
		}
		return 1;
	}

	// Leave critical section
	void* pLeaveFn = D2MOO_Resolve("g_pfnLeaveCritSec");
	if (pLeaveFn && pCritSec)
	{
		((void(*)(void*))pLeaveFn)(pCritSec);
	}
	return 0;
}
