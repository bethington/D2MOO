#include "D2Debugger.h"
#include <Windows.h>

// Standalone debugger window (D2Debugger.imgui.d3d9.cpp): own thread + own D3D9
// device + own top-most window -- renderer-agnostic, so it shows regardless of
// PD2's (un-hookable) present path. Replaces the present-hook overlay attempt,
// which PD2's wrapped renderer defeats (see LIVE_DISPATCH_FRAMEWORK_PLAN.md Ph4).
void D2Debugger_StartStandalone();

// NOLINTBEGIN(bugprone-branch-clone)
BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// Kick off the standalone debugger window on its own thread -- independent
		// of the (dead-on-PD2) GAME_UpdateProgress hook and of the game's renderer.
		D2Debugger_StartStandalone();
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}

	return TRUE;
}
// NOLINTEND(bugprone-branch-clone)
