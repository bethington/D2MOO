// D2Debugger.gamewindow.cpp -- get the real game window out of the way now that
// the game renders inside the debugger's panel.
//
// THE CONSTRAINT THAT DECIDES EVERYTHING. The panel is fed by the game calling
// GDI StretchBlt ~25 times a second into its OWN window (see
// D2Debugger.vcapture.cpp). Anything that stops the game presenting blanks the
// panel -- and "window visible but drawing nothing" is exactly the failure that
// cost hours earlier, so every mode here is measured against the present rate
// rather than assumed to be harmless.
//
// MODES, in increasing order of how much they interfere with the renderer:
//   OFFSCREEN  moves the window to -10000,-10000. It stays visible by every
//              test the app or compositor applies, so the present path is
//              untouched. Least likely to break anything.
//   LAYERED    keeps it in place at alpha 0. Composites to nothing, but routes
//              painting through DWM's redirection surface -- the layer this
//              GDI path otherwise bypasses.
//   HIDE       ShowWindow(SW_HIDE). Tidiest if it survives; a hidden window has
//              no presentation target and many renderers short-circuit on that.
//
// WHY IN-PROCESS. The game runs ELEVATED, so SetWindowPos/SetWindowLong from an
// unelevated process are silently dropped by UIPI -- the trap that forced
// fun-doc's _set_borderless to read the style back instead of trusting the
// call. D2Debugger lives inside Game.exe, so there is no privilege boundary and
// no read-back dance.
//
// Everything is restored on toggle-off and at shutdown. A game window parked at
// -10000 with no way back is worse than one that is merely in the way.

#include <windows.h>
#include <atomic>

// The debugger's render thread declares itself per-monitor DPI aware so its own
// window can be a true 2560x1440 (see D2Debugger.hostwindow.cpp). This file
// touches the GAME's window, which is not: it is created by unaware game
// threads and driven from outside by fun-doc's game_window.py, also unaware.
// Every rect here is therefore taken and put back in UNAWARE coordinates. Mixed
// spaces would be off by exactly the scale factor -- 1.25 on this box -- so a
// rect saved from the panel's checkbox and restored from the oracle's HTTP
// thread would put the window back 25% away from where it was.
extern "C" void* D2Host_PushUnaware();
extern "C" void  D2Host_PopUnaware(void* token);

namespace
{
	struct UnawareScope
	{
		void* tok;
		UnawareScope() : tok(D2Host_PushUnaware()) {}
		~UnawareScope() { D2Host_PopUnaware(tok); }
	};
	HWND    g_hwnd = nullptr;
	bool    g_saved = false;
	RECT    g_rect{};
	LONG    g_exStyle = 0;
	int     g_mode = 0;              // 0 none, 1 offscreen, 2 layered, 3 hide
	std::atomic<int> g_applied{ 0 };

	HWND GameWindow()
	{
		HWND h = FindWindowA(nullptr, "Diablo II");
		// Never target our own window -- hiding the debugger to hide the game
		// would be a memorable way to lose access to the toggle.
		if (h && h != g_hwnd)
			return h;
		return h;
	}

	void SaveOnce(HWND h)
	{
		if (g_saved)
			return;
		UnawareScope unaware;
		GetWindowRect(h, &g_rect);
		g_exStyle = GetWindowLongA(h, GWL_EXSTYLE);
		g_saved = true;
	}
}

// mode: 0 restore, 1 offscreen, 2 layered, 3 hide
extern "C" void D2AudioCap_SetSuppressNative(int on);

extern "C" int D2GameWindow_SetMode(int mode)
{
	// Hidden => the game's own audio is silenced. Set BEFORE the window lookup
	// on purpose: the mute must still apply if the window cannot be found, and
	// it lives here rather than at the call sites so no caller -- the panel's
	// checkbox, its boot path, or the oracle's endpoints -- can drift from it.
	D2AudioCap_SetSuppressNative(mode != 0 ? 1 : 0);

	HWND h = GameWindow();
	if (!h)
		return 0;
	SaveOnce(h);

	// Same coordinate space as SaveOnce, for the whole of the apply below --
	// the restore path replays g_rect and must read it back as it was written.
	UnawareScope unaware;

	// Always come back to a known state before applying the next mode, so
	// switching between them cannot leave a layered style stacked on an
	// off-screen position.
	SetWindowLongA(h, GWL_EXSTYLE, g_exStyle);
	ShowWindow(h, SW_SHOW);
	SetWindowPos(h, nullptr, g_rect.left, g_rect.top, 0, 0,
	             SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

	switch (mode)
	{
	case 1:
		// Off-screen, and out of the taskbar/Alt-Tab so it cannot be brought
		// back by accident. WS_EX_TOOLWINDOW only takes effect across a
		// hide/show cycle.
		ShowWindow(h, SW_HIDE);
		SetWindowLongA(h, GWL_EXSTYLE,
		               (g_exStyle | WS_EX_TOOLWINDOW) & ~WS_EX_APPWINDOW);
		ShowWindow(h, SW_SHOW);
		SetWindowPos(h, nullptr, -10000, -10000, 0, 0,
		             SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		break;
	case 2:
		SetWindowLongA(h, GWL_EXSTYLE, g_exStyle | WS_EX_LAYERED);
		SetLayeredWindowAttributes(h, 0, 0, LWA_ALPHA);
		break;
	case 3:
		ShowWindow(h, SW_HIDE);
		break;
	default:
		mode = 0;
		break;
	}
	g_mode = mode;
	g_applied.store(mode, std::memory_order_relaxed);
	return 1;
}

extern "C" int D2GameWindow_Mode() { return g_applied.load(std::memory_order_relaxed); }

// Called from the debugger's teardown. Leaving the game window parked off-screen
// after the debugger exits would strand it with nothing left to restore it.
extern "C" void D2GameWindow_Restore()
{
	if (g_saved && g_mode)
		D2GameWindow_SetMode(0);
}
