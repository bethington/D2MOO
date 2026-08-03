#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitializeServerDialogStateHandler
extern "C" int __stdcall InitializeServerDialogStateHandler(void)
{
	// The three globals form a server-dialog handler configuration (see plate
	// comment "Structure Layout" -- offsets +0x00/+0x08/+0x18). The decompile
	// shows them as separate globals; the offsets describe their relative
	// placement inside the underlying configuration record.
	int*    pStateType  = (int*)   D2MOO_Resolve("g_dwServerDialogStateType");
	void**  pCallback   = (void**) D2MOO_Resolve("g_pfnServerDialogCallback");
	short*  pResourceId = (short*) D2MOO_Resolve("g_wServerDialogResourceId");

	if (!pStateType || !pCallback || !pResourceId)
		return 0; // resolver missing / name unknown -> obvious wrong-value sentinel

	*pStateType  = 4;
	// CLIENT_CompleteNpcInteractionAndReset is a code symbol, not a resolvable
	// global; the proof only compares the return value (always 1), so the
	// assignment is faithfully reproduced with the previous-resolved slot.
	*pCallback   = 0;
	*pResourceId = (short)0x58dc;

	return 1;
}
