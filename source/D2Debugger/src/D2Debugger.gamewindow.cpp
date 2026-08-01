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

namespace
{
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
		GetWindowRect(h, &g_rect);
		g_exStyle = GetWindowLongA(h, GWL_EXSTYLE);
		g_saved = true;
	}
}

// mode: 0 restore, 1 offscreen, 2 layered, 3 hide
extern "C" int D2GameWindow_SetMode(int mode)
{
	HWND h = GameWindow();
	if (!h)
		return 0;
	SaveOnce(h);

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
