#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_SendPartyInviteResetPacket
extern "C" uint32_t __stdcall CLIENT_SendPartyInviteResetPacket()
{
	// Resolve the three globals exposed by the resolver. The remaining message
	// payload bytes (g_bExpansionVersionCheck_1, cRam6fbc971f, cRam6fbc9720)
	// are not in the resolver list and are flagged as NEEDS GLOBAL below.
	uint8_t* pInteractionActive = (uint8_t*)D2MOO_Resolve("g_bInteractionActive");
	void**         pPartyInviteDialog = (void**)D2MOO_Resolve("g_pPartyInviteConfirmDialog");
	uint8_t* pExpansionCheck    = (uint8_t*)D2MOO_Resolve("g_bExpansionVersionCheck");

	if (!pInteractionActive || !pPartyInviteDialog || !pExpansionCheck)
		return 0; // resolver misconfig / unknown name -> obvious mismatch

	// Mirror the decompile's reads so we touch the same live globals.
	uint8_t bInteraction = *pInteractionActive;
	void*         pDialog      = *pPartyInviteDialog;
	uint8_t bExpansion   = *pExpansionCheck;

	// The conditional cleanup branch invokes ProcessGameEndCleanup() and
	// CLIENT_ResetGameStateGlobals() (both undefined in this provider) and
	// also writes NULL into g_pPartyInviteConfirmDialog. Skipped per the
	// "no undefined function calls" and "read-only / no global mutation"
	// rules. Likewise the unconditional send calls the undefined
	// CLIENT_SendChatMessageThrottled(szPartyInviteMsg). The function
	// unconditionally returns 1 in every branch, so the proof against the
	// live original reduces to comparing the uint return.

	// NEEDS GLOBAL: g_bExpansionVersionCheck_1
	// NEEDS GLOBAL: cRam6fbc971f
	// NEEDS GLOBAL: cRam6fbc9720

	(void)bInteraction;
	(void)pDialog;
	(void)bExpansion;
	return 1;
}
