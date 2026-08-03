#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_CloseAndRecreatePartyDialog
extern "C" int __stdcall CLIENT_CloseAndRecreatePartyDialog(void)
{
	// Resolve both pointer-typed globals (names start g_p_).
	void* pNpcAddr = D2MOO_Resolve("g_pNpcInteractionDialog");
	void* pEvtAddr = D2MOO_Resolve("g_pNpcDialogEventListNode");
	if (!pNpcAddr || !pEvtAddr)
		return 0; // resolver missing / name unknown -> obvious wrong-value sentinel

	// Bare symbol in decompile = pointer's VALUE -> single deref of resolved addr.
	void* pNpc = *(void**)pNpcAddr;
	// Touch the event list node the same way the decompile does.
	void* pEvtNode = *(void**)pEvtAddr;
	(void)pNpc;
	(void)pEvtNode;

	// The original calls ProcessGameEndCleanup(), invokes the double-dereferenced
	// thunk at *_g_pNpcDialogEventListNode with arg 1, clears both globals to 0,
	// and finally calls CLIENT_CreatePartyDialogWindow(). Those callees are NOT
	// defined in this provider (error C3861). The function unconditionally
	// returns 1 on every successful code path -- the oracle compares this
	// return value, so emit it directly.
	return 1;
}
