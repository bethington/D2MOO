#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SetGameEventTimerState
extern "C" void __stdcall SetGameEventTimerState()
{
	int in_EAX;
	__asm mov in_EAX, eax;

	char* state_base = (char*)D2MOO_Resolve("g_bConfirmDlgRenderState");
	void** callback_var = (void**)D2MOO_Resolve("g_pfnConfirmDlgRenderCallback");
	void** render_arr  = (void**)D2MOO_Resolve("g_pfnRenderMainGameFrame");
	if (!state_base || !callback_var || !render_arr) return;

	*state_base  = (char)in_EAX;
	*callback_var = render_arr[in_EAX];
}
