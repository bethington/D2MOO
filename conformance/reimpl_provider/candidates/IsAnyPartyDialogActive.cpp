#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: IsAnyPartyDialogActive
extern "C" int __stdcall IsAnyPartyDialogActive(void)
{
	// Resolve each pointer-variable global (all are g_p*). Dereference once to
	// obtain the pointer VALUE that the decompile compares to NULL.
	void* res_g_pGambleDialog              = D2MOO_Resolve("g_pGambleDialog");
	void* res_g_pNpcChatDialog             = D2MOO_Resolve("g_pNpcChatDialog");
	void* res_g_pPartyInviteDialog_975f    = D2MOO_Resolve("g_pPartyInviteDialog_6fbc975f");
	void* res_g_pPartyInviteDialog_9763    = D2MOO_Resolve("g_pPartyInviteDialog_6fbc9763");
	void* res_g_pPartyRequestDialog        = D2MOO_Resolve("g_pPartyRequestDialog");
	void* res_g_pPartyInviteConfirmDialog  = D2MOO_Resolve("g_pPartyInviteConfirmDialog");
	void* res_g_pNpcInteractionDialog      = D2MOO_Resolve("g_pNpcInteractionDialog");
	void* res_g_pPlayerInviteDialog        = D2MOO_Resolve("g_pPlayerInviteDialog");

	// Guard: any unresolved symbol -> obvious wrong-value sentinel
	if (!res_g_pGambleDialog || !res_g_pNpcChatDialog ||
	    !res_g_pPartyInviteDialog_975f || !res_g_pPartyInviteDialog_9763 ||
	    !res_g_pPartyRequestDialog || !res_g_pPartyInviteConfirmDialog ||
	    !res_g_pNpcInteractionDialog || !res_g_pPlayerInviteDialog) {
		return -1;
	}

	// Dereference once -> the bare pointer values the decompile uses
	void* g_pGambleDialog             = (void*)*(void**)res_g_pGambleDialog;
	void* g_pNpcChatDialog            = (void*)*(void**)res_g_pNpcChatDialog;
	void* g_pPartyInviteDialog_6fbc975f = (void*)*(void**)res_g_pPartyInviteDialog_975f;
	void* g_pPartyInviteDialog_6fbc9763 = (void*)*(void**)res_g_pPartyInviteDialog_9763;
	void* g_pPartyRequestDialog       = (void*)*(void**)res_g_pPartyRequestDialog;
	void* g_pPartyInviteConfirmDialog = (void*)*(void**)res_g_pPartyInviteConfirmDialog;
	void* g_pNpcInteractionDialog     = (void*)*(void**)res_g_pNpcInteractionDialog;
	void* g_pPlayerInviteDialog       = (void*)*(void**)res_g_pPlayerInviteDialog;

	// Translate decompile literally -- one compound condition over all eight dialog
	// pointers; if ALL are NULL, return false (0), otherwise true (1).
	if ((((((g_pGambleDialog == (void*)0) && (g_pNpcChatDialog == (void*)0)) &&
	       (g_pPartyInviteDialog_6fbc975f == (void*)0)) &&
	      ((g_pPartyInviteDialog_6fbc9763 == (void*)0 && (g_pPartyRequestDialog == (void*)0)))) &&
	     ((g_pPartyInviteConfirmDialog == (void*)0 &&
	      ((g_pNpcInteractionDialog == (void*)0 &&
	       (g_pPlayerInviteDialog == (void*)0))))))) {
		return 0;
	}
	return 1;
}
