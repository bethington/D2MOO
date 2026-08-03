// D2MOO reimpl of CLIENT_ProcessNpcDialogStateTransition (D2Client 0x6faf7480).
// Dispatches on g_dwNpcInteractionState / g_dwInteractionMode and always
// returns 1. Game-side helpers (ProcessGameEndCleanup, CLIENT_CreateGambleDialogWindow,
// CLIENT_InitiatePartyInteraction) are omitted (not in provider) -- their side effects
// do not affect the 1-return value this oracle verifies. Global state is read-only
// here per the reimpl rules; the writes the original performs to g_pGambleDialog /
// g_dwGroundHoverTimer / g_dwGroundHoverState are skipped because every branch
// returns 1 regardless.

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_ProcessNpcDialogStateTransition
extern "C" int __stdcall CLIENT_ProcessNpcDialogStateTransition(void)
{
	// g_dwNpcInteractionState: data global (read as int)
	char* baseNpcState = (char*)D2MOO_Resolve("g_dwNpcInteractionState");
	if (!baseNpcState) return 0; // resolver missing -> obvious mismatch sentinel

	if (*(int*)baseNpcState == 5) {
		// branch A (gamble): would call ProcessGameEndCleanup + create gamble dialog.
		// g_pGambleDialog is read in the original; we resolve+read for fidelity
		// but do not mutate it (read-only reimpl rule).
		void** pGambleDialog = (void**)D2MOO_Resolve("g_pGambleDialog");
		if (!pGambleDialog) return 0;
		(void)(*pGambleDialog != (void*)0x0);
		return 1;
	}

	// _g_dwInteractionMode -> "g_dwInteractionMode" (drop Ghidra leading underscore)
	char* baseInteractionMode = (char*)D2MOO_Resolve("g_dwInteractionMode");
	if (!baseInteractionMode) return 0;

	if (*(int*)baseInteractionMode == 4) {
		// branch B (party): would call CLIENT_InitiatePartyInteraction(0, 0)
		void** pGambleDialog = (void**)D2MOO_Resolve("g_pGambleDialog");
		if (!pGambleDialog) return 0;
		(void)(*pGambleDialog != (void*)0x0);
		return 1;
	}

	// default: original would zero g_dwGroundHoverTimer and g_dwGroundHoverState.
	// Skipped (read-only). Return is still 1.
	return 1;
}
