#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_ResetVideoStateVariables
extern "C" void __stdcall CLIENT_ResetVideoStateVariables(void)
{
	char* base;

	// 1. g_dwUIPanelState = 0x10
	base = (char*)D2MOO_Resolve("g_dwUIPanelState");
	if (!base) return;
	*(int*)base = 0x10;

	// 2. g_dwLastError_6fbcc3e0 = 0
	base = (char*)D2MOO_Resolve("g_dwLastError_6fbcc3e0");
	if (!base) return;
	*(int*)base = 0;

	// 3. g_pPlayerName_6fbcc3fc = (pointer slot) -> &g_MISSILE_pDefaultAnimSeqData
	base = (char*)D2MOO_Resolve("g_pPlayerName_6fbcc3fc");
	if (!base) return;
	// NEEDS GLOBAL: g_MISSILE_pDefaultAnimSeqData
	{
		void* target = D2MOO_Resolve("g_MISSILE_pDefaultAnimSeqData");
		if (!target) return;
		*(void**)base = target;
	}

	// 4. g_dwVideoInitEnabled_6fbcc3e4 = 0
	base = (char*)D2MOO_Resolve("g_dwVideoInitEnabled_6fbcc3e4");
	if (!base) return;
	*(int*)base = 0;

	// 5. _g_dwGroundHoverTimer = 0  (underscore = overlap; resolve as g_dwGroundHoverTimer)
	base = (char*)D2MOO_Resolve("g_dwGroundHoverTimer");
	if (!base) return;
	*(int*)base = 0;
}
