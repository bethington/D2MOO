#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_SetViewSpriteAnimState
extern "C" void __fastcall CLIENT_SetViewSpriteAnimState(int nAnimStateOrFrame, int in_EAX)
{
	// nAnimStateOrFrame arrives in ECX; in_EAX arrives in EAX (register_explicit).
	// Result is written as a side effect to the global g_dwViewAnimProgress.
	int* base = (int*)D2MOO_Resolve("g_dwViewAnimProgress");
	if (!base)
		return; // resolver missing -> bail so a misconfig fails loudly rather than matching by accident

	if (nAnimStateOrFrame == 0) {
		*base = nAnimStateOrFrame; // == 0
		return;
	}
	*base = (in_EAX << 8) / nAnimStateOrFrame;
}
