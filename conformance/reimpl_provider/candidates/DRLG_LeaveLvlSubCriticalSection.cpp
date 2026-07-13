#include "../provider_runtime.h"

// NEEDS GLOBAL: g_pLvlSubCritSect
// NEEDS GLOBAL: g_pfnLogExitCriticalSection

// D2MOO_REIMPL_EXPORT: DRLG_LeaveLvlSubCriticalSection
extern "C" void __stdcall DRLG_LeaveLvlSubCriticalSection(void)
{
	// Both globals required by this function (g_pLvlSubCritSect,
	// g_pfnLogExitCriticalSection) are NOT in the verified D2MOO_Resolve
	// table -- the resolver returns NULL for them, so the original
	// algorithm (read pointer, call LeaveCriticalSection via function
	// pointer) cannot be reproduced against live game state.
	//
	// Original (spec):
	//   if (g_pLvlSubCritSect != NULL)
	//       g_pfnLogExitCriticalSection(g_pLvlSubCritSect);
	//
	// Cannot resolve -> cannot reach the call. Function returns void
	// so there is no wrong-value sentinel to return; this is a no-op
	// stub until the missing globals are added to the resolve table.
}
