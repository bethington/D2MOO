#include "D2Debugger.h"
#include <Windows.h>

// Standalone debugger window (D2Debugger.imgui.d3d9.cpp): own thread + own D3D9
// device + own top-most window -- renderer-agnostic, so it shows regardless of
// PD2's (un-hookable) present path. Replaces the present-hook overlay attempt,
// which PD2's wrapped renderer defeats (see LIVE_DISPATCH_FRAMEWORK_PLAN.md Ph4).
void D2Debugger_StartStandalone();

// Virtual input (d2-fleet/patch/vinput.cpp, shared with D2FleetHook):
// Detours-hooks the cursor/key APIs the game actually reads, so it can be driven
// without moving the operator's real pointer. Installed here (DllMain is a
// supported Detours attach context, and user32 is always loaded by then) but
// INERT until explicitly enabled -- with virtual mode off every hook is a
// straight pass-through.
#include "vinput.h"
#include "gamewindow.h"

// The services the shared game-window layer needs from THIS application and
// cannot assume: an always-on-top window to yield, a DPI-unaware scope to take
// rects in, the panel's own enter/leave, and where the image lands inside a
// letterboxed client. D2FleetHook supplies none of them -- its console is in
// another process -- which is exactly why they are registered rather than
// linked.
extern "C" void* D2Host_PushUnaware();
extern "C" void  D2Host_PopUnaware(void* token);
extern "C" void  D2Host_SetTopmost(int on);
extern "C" void  D2GamePanel_SetGameFullscreen(int on);
extern "C" int   D2Probe_LetterboxRect(int* x, int* y, int* w, int* h,
                                       int* clientW, int* clientH);
extern "C" int   D2Probe_SourceSize(int* w, int* h);

static void RegisterGameWindowHost()
{
	D2GameWindowHost host = {};
	host.PushUnaware        = D2Host_PushUnaware;
	host.PopUnaware         = D2Host_PopUnaware;
	host.SetTopmost         = D2Host_SetTopmost;
	host.SetPanelFullscreen = D2GamePanel_SetGameFullscreen;
	host.LetterboxRect      = D2Probe_LetterboxRect;
	host.SourceSize         = D2Probe_SourceSize;
	// Where F11 leaves the window: hidden, which is what the panel has assumed
	// since the "Hide game" checkbox was retired.
	host.restingMode        = D2GAMEWINDOW_HIDDEN;
	D2GameWindow_SetHost(&host);
}


// Present-path probe (D2Debugger.probe.cpp). It owns every GDI/GL detour and
// is also what feeds frame capture: PD2 presents via gdi32!StretchBlt from
// D2Gdi.dll (measured), so capture has no detours of its own.
extern "C" void D2Probe_StartInstall();

// Live-game patch system (D2Debugger.patchcore.cpp). Claims this process's
// instance slot, loads d2patches.json, applies the profile's binary redirects,
// and runs every enabled EARLY patch -- multi_instance, exception_passthru,
// anti_anti_debug and the pre-main debugger gate all need to be in force BEFORE
// the game initializes, which is only reachable from here in DllMain. The
// remaining stages run later, from the debugger's own thread
// (D2Patch_InstallDeferred), once the game's DLLs are mapped.
extern "C" void D2Patch_InstallEarly(void);

// NOLINTBEGIN(bugprone-branch-clone)
BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// FIRST, synchronously, before any thread is created: claim the instance
		// slot and apply the EARLY patches. multi_instance must beat D2gfx's
		// single-instance check, the exception/anti-debug patches must be in
		// place before the game can fault or check for a debugger, and the slot
		// decides which control port the server below will bind. This is the only
		// stage reachable before the game starts initializing.
		D2Patch_InstallEarly();
		// Kick off the standalone debugger window on its own thread -- independent
		// of the (dead-on-PD2) GAME_UpdateProgress hook and of the game's renderer.
		D2Debugger_StartStandalone();
		// BEFORE anything can change a window mode: the shared layer falls back
		// to hostless defaults until this lands, and a fullscreen applied in
		// that window would forget to yield topmost.
		RegisterGameWindowHost();
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
