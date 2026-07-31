#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_CloseTextSliderCelHandle
extern "C" void __stdcall CLIENT_CloseTextSliderCelHandle(void)
{
	// g_dwTextSliderCel is a data global (g_dw* prefix) -- resolve returns its address directly.
	char* base = (char*)D2MOO_Resolve("g_dwTextSliderCel");
	if (!base) return; // resolver missing -- fail loud, do not match by accident

	int dwPrevTextSliderCel = *(int*)(base + 0x0);
	if (*(int*)(base + 0x0) != 0) {
		// D2Client_ReleaseHotkeyResourceCallbacks() -- helper not in provider, skipped
		if (dwPrevTextSliderCel != 0) {
			// CLIENT_InitializeModule() -- helper not in provider, skipped
		}
		*(int*)(base + 0x0) = 0;
	}
}
