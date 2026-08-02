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
// TWO THREADS CALL IN HERE, which is what the lock below is for: the debugger's
// render thread (the panel's checkbox and its boot-apply) and the HTTP control
// thread (POST /window/mode is handled inline on ServerThread -- it is not
// marshalled onto anything).
//
// Everything is restored on toggle-off and at shutdown. A game window parked at
// -10000 with no way back is worse than one that is merely in the way.

#include <windows.h>
#include <atomic>
#include <mutex>

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

	// Serialises the whole save-then-apply sequence against the second caller.
	//
	// THE RACE THIS CLOSES IS NOT A STYLE POINT. The save latch used to be a
	// plain bool, so both threads could read it unset: one applies OFFSCREEN and
	// moves the window to -10000,-10000, and the other's GetWindowRect then
	// captures -10000 AS THE ORIGINAL and latches it. Every restore afterwards
	// faithfully puts the window back to -10000 -- the stranded window this file
	// exists to prevent, and unrecoverable short of restarting the game.
	//
	// The cost is that a caller can now wait on a caller that is itself waiting
	// on the game's message pump (ShowWindow/SetWindowPos reach across to the
	// thread that owns the window). That was already true of the Win32 calls
	// themselves -- this only widens it from the calling thread to both -- and a
	// brief stall against a wedged game is a far better trade than a window that
	// can never come back.
	std::mutex g_lock;

	// The saved geometry belongs to a SPECIFIC window, so it is keyed by one.
	// D2 destroys and recreates its window on a resolution or windowed/
	// fullscreen change; a latch that only ever armed once would then replay a
	// dead window's rect and ex-style onto the new one. A different HWND
	// re-saves instead of inheriting.
	HWND    g_savedHwnd = nullptr;
	RECT    g_rect{};
	LONG    g_exStyle = 0;
	// 0 none, 1 offscreen, 2 layered, 3 hide.
	//
	// ONE value, atomic, read from any thread. There were two copies of this --
	// a plain int for in-file use and an atomic for callers -- kept in step by
	// hand on a single line. Nothing had gone wrong with it yet, but a second
	// copy of a value is the drift the panel's own options were consolidated to
	// avoid, and this one is read by the input layer to decide whether to
	// swallow the game's cursor clip.
	std::atomic<int> g_mode{ 0 };

	HWND GameWindow()
	{
		// By exact title, which is also what keeps the debugger's own window out
		// of reach: the host window is created titled "D2Debugger"
		// (D2Debugger.imgui.d3d9.cpp), so it can never match here. There used to
		// be an explicit "never target our own window" guard as well, but it
		// tested against an HWND that was never assigned and both of its
		// branches returned the same thing -- a protection that existed only in
		// its comment. The title lookup is the real one.
		return FindWindowA(nullptr, "Diablo II");
	}

	// Caller holds g_lock.
	void SaveOnce(HWND h)
	{
		if (g_savedHwnd == h)
			return;
		UnawareScope unaware;
		GetWindowRect(h, &g_rect);
		g_exStyle = GetWindowLongA(h, GWL_EXSTYLE);
		g_savedHwnd = h;
	}
}

// mode: 0 restore, 1 offscreen, 2 layered, 3 hide
//
// No audio side-effect any more. Hiding the window used to have to silence the
// game's own DirectSound output as well, or you heard a window you could not
// see. dsound-headless renders to no device at all, so there is nothing left to
// silence -- what you hear is our mixer, and that is governed by the panel's
// Audio option, not by whether the game window is visible.
extern "C" int D2GameWindow_SetMode(int mode)
{
	std::lock_guard<std::mutex> guard(g_lock);

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
	g_mode.store(mode, std::memory_order_relaxed);
	return 1;
}

extern "C" int D2GameWindow_Mode() { return g_mode.load(std::memory_order_relaxed); }

// Called from the debugger's teardown. Leaving the game window parked off-screen
// after the debugger exits would strand it with nothing left to restore it.
extern "C" void D2GameWindow_Restore()
{
	// Deliberately takes no lock: SetMode takes it and g_lock is not recursive.
	// A non-zero mode is proof the save already happened, since SetMode saves
	// before it ever stores one -- so there is no separate saved-flag to test.
	if (D2GameWindow_Mode() != 0)
		D2GameWindow_SetMode(0);
}
