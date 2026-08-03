#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_CloseNPCInteract
extern "C" int __stdcall CLIENT_CloseNPCInteract(void)
{
	// g_pPartyInviteDialog_6fbc9763 is a pointer variable (g_p* prefix).
	// STEP 1: deref the resolved address ONCE to get the pointer's value.
	void** ppPartyInviteDialog = (void**)D2MOO_Resolve("g_pPartyInviteDialog_6fbc9763");
	if (!ppPartyInviteDialog)
		return 0; // resolver missing / name unknown -> obvious mismatch sentinel

	if (*ppPartyInviteDialog != (void*)0) {
		// ProcessGameEndCleanup() is not provided in the provider; its side
		// effects are not observable in this function's return value.
		*ppPartyInviteDialog = (void*)0;
	}
	// CLIENT_ResetGameStateGlobals() is not provided in the provider; its
	// side effects are not observable in this function's return value.
	return 1;
}
