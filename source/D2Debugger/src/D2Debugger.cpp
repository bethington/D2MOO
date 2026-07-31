#include "D2Debugger.h"
#include <Windows.h>

// Standalone debugger window (D2Debugger.imgui.d3d9.cpp): own thread + own D3D9
// device + own top-most window -- renderer-agnostic, so it shows regardless of
// PD2's (un-hookable) present path. Replaces the present-hook overlay attempt,
// which PD2's wrapped renderer defeats (see LIVE_DISPATCH_FRAMEWORK_PLAN.md Ph4).
void D2Debugger_StartStandalone();

// Virtual input (D2Debugger.vinput.cpp): Detours-hooks the cursor/key APIs the
// game actually reads, so it can be driven without moving the operator's real
// pointer. Installed here (DllMain is a supported Detours attach context, and
// user32 is always loaded by then) but INERT until explicitly enabled -- with
// virtual mode off every hook is a straight pass-through.
extern "C" void D2VInput_Install();


// Present-path probe (D2Debugger.probe.cpp). It owns every GDI/GL detour and
// is also what feeds frame capture: PD2 presents via gdi32!StretchBlt from
// D2Gdi.dll (measured), so capture has no detours of its own.
extern "C" void D2Probe_StartInstall();

// NOLINTBEGIN(bugprone-branch-clone)
BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// Kick off the standalone debugger window on its own thread -- independent
		// of the (dead-on-PD2) GAME_UpdateProgress hook and of the game's renderer.
		D2Debugger_StartStandalone();
		D2VInput_Install();
		D2Probe_StartInstall();
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
