#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ClearTextQueueEntries
extern "C" void __stdcall ClearTextQueueEntries(void)
{
	// g_pTextQueueHead / g_pSubtitleListHead share address 0x6fbc3aa4.
	// _g_pSubtitleListHead in the decompile is the pointer's VALUE -- D2MOO_Resolve
	// returns &g_pTextQueueHead, so we deref once to obtain the list head.
	void* vp = D2MOO_Resolve("g_pTextQueueHead");
	if (!vp)
		return; // resolver missing / symbol unknown -> obvious no-op (oracle runs live).
	int iVar1 = *(int*)vp;
	for (; iVar1 != 0; iVar1 = *(int*)((char*)iVar1 + 0x22)) {
		*(int*)((char*)iVar1 + 0x1e) = 0;
	}
}
