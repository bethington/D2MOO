#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitAssertCallbackWithRecording
// Decompile shows `g_pfnAssertCallback = CLIENT_CreatePartyStatsDialog;`.
// `CLIENT_CreatePartyStatsDialog` is the Ghidra-displayed (module-prefixed) name
// for the function pointer global D2MOO knows as `g_pfnCreatePartyStatsDialog`.
// Reading *g_pfnCreatePartyStatsDialog reproduces the same stored function
// address the original writes into g_pfnAssertCallback.
// NEEDS GLOBAL: CLIENT_CreatePartyStatsDialog
extern "C" int __stdcall InitAssertCallbackWithRecording(void)
{
	int* pType = (int*)D2MOO_Resolve("g_dwAssertCallbackType");
	if (!pType) return 0;
	*pType = 3;

	void** pCreate = (void**)D2MOO_Resolve("g_pfnCreatePartyStatsDialog");
	void** pAssert = (void**)D2MOO_Resolve("g_pfnAssertCallback");
	if (!pCreate || !pAssert) return 0;
	*pAssert = *pCreate;

	short* pResId = (short*)D2MOO_Resolve("g_wAssertCallbackResId");
	if (!pResId) return 0;
	*pResId = (short)0xd45;

	return 1;
}
