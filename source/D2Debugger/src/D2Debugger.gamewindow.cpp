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
//   FULLSCREEN the opposite of the other three: the window is SHOWN, borderless,
//              filling its monitor and in front. This is the one mode you are
//              meant to look at -- see D2GamePanel_SetGameFullscreen, which
//              wraps it with the input and Z-order work that makes it usable.
//
// FULLSCREEN IS NOT "CONCEALED". Three of these four modes put the window out of
// sight and one deliberately does not, so callers asking "is the game hidden?"
// must use D2GameWindow_IsConcealed rather than testing the mode against 0. The
// cursor-clip swallow in D2Debugger.vinput.cpp is exactly such a caller, and
// getting it wrong there means a fullscreen game cannot confine your pointer.
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
// Yield / reclaim the debugger's always-on-top status (D2Debugger.hostwindow.cpp).
// A topmost debugger is why showing the real window used to accomplish nothing:
// the game came back exactly where it was, underneath.
extern "C" void D2Host_SetTopmost(int on);
// The full enter/leave, owned by the panel because it also parks the virtual
// input layer (D2Debugger.gamepanel.cpp). Called from the game-side F11 filter
// below, which is the only way out once the game holds keyboard focus.
extern "C" void D2GamePanel_SetGameFullscreen(int on);
// Apply/release a REAL cursor clip, bypassing our own ClipCursor hook
// (D2Debugger.vinput.cpp) -- that hook exists to swallow the GAME's clips, so
// going through it would swallow ours too.
extern "C" int D2VInput_ClipCursorReal(const void* rect);
// Where the image lands inside the client while full screen
// (D2Debugger.probe.cpp). 0 until a full-screen blit has been observed.
extern "C" int D2Probe_LetterboxRect(int* x, int* y, int* w, int* h,
                                     int* clientW, int* clientH);
// The size the game is currently rasterising (800x600 at menus, 1068x600
// in-world), which is what the cursor clip is scoped to while full screen.
extern "C" int D2Probe_SourceSize(int* w, int* h);

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
	// GWL_STYLE as well as GWL_EXSTYLE, because FULLSCREEN is the first mode
	// that touches it -- borderless means clearing WS_CAPTION and WS_THICKFRAME,
	// and a restore that only put the ex-style back would hand you the game
	// permanently stripped of its title bar.
	LONG    g_style = 0;
	// 0 none, 1 offscreen, 2 layered, 3 hide, 4 fullscreen.
	//
	// ONE value, atomic, read from any thread. There were two copies of this --
	// a plain int for in-file use and an atomic for callers -- kept in step by
	// hand on a single line. Nothing had gone wrong with it yet, but a second
	// copy of a value is the drift the panel's own options were consolidated to
	// avoid, and this one is read by the input layer to decide whether to
	// swallow the game's cursor clip.
	std::atomic<int> g_mode{ 0 };
	// The window while it is full screen, cached so the cursor clip can be
	// re-asserted from the render thread without taking g_lock on a path that
	// runs every frame.
	std::atomic<void*> g_fsHwnd{ nullptr };

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
		g_style   = GetWindowLongA(h, GWL_STYLE);
		g_savedHwnd = h;
	}

	// ---- game-side F11 ------------------------------------------------------
	//
	// The debugger's own F11 handler is on the DEBUGGER's window proc, so the
	// moment the game goes fullscreen and takes focus it stops seeing the key --
	// and a fullscreen game you cannot leave is a game you have to kill. This
	// filter is the other half: whichever window has focus, its own proc catches
	// F11, so exactly one of the two always does.
	//
	// In-process, so no UIPI boundary and no marshalling. Chained, never
	// replaced. ANSI vs Unicode is chosen from the window itself: installing a
	// W proc on an A window (or the reverse) silently corrupts the text of every
	// character message the game receives.
	WNDPROC g_origProc = nullptr;
	HWND    g_hookedHwnd = nullptr;
	bool    g_hookedUnicode = false;

	// Map a client-space mouse position back through the letterbox.
	//
	// D2 works out the cursor from its CLIENT RECT -- established by measurement,
	// not assumption: clicks land correctly full screen at 1068x600, which they
	// could not if it took raw 1:1 coordinates in a 2048-wide client. So the
	// moment the image is inset by bars, every click is out by the inset unless
	// it is undone here. In-world (16:9 into 16:9) the rect fills the client and
	// this is an exact identity, so nothing is disturbed where nothing is wrong.
	//
	// Coordinates OUTSIDE the image are clamped to its edge rather than passed
	// through: the mirrored surround is decoration, and a click on it should mean
	// the nearest real pixel, not a position in the game world that is not shown.
	bool MapMouseThroughLetterbox(LPARAM in, LPARAM* out)
	{
		int lx = 0, ly = 0, lw = 0, lh = 0, cw = 0, ch = 0;
		if (!D2Probe_LetterboxRect(&lx, &ly, &lw, &lh, &cw, &ch))
			return false;
		if (lw <= 0 || lh <= 0 || cw <= 0 || ch <= 0)
			return false;
		if (lx == 0 && ly == 0 && lw == cw && lh == ch)
			return false;                      // exact fit -- identity, leave it

		const int px = (short)LOWORD(in);
		const int py = (short)HIWORD(in);
		// Position within the drawn image, then re-expressed against the full
		// client, which is the space D2 will scale from.
		long long ix = (long long)(px - lx) * cw / lw;
		long long iy = (long long)(py - ly) * ch / lh;
		if (ix < 0) ix = 0; else if (ix > cw - 1) ix = cw - 1;
		if (iy < 0) iy = 0; else if (iy > ch - 1) iy = ch - 1;
		*out = MAKELPARAM((int)ix, (int)iy);
		return true;
	}

	bool IsClientMouseMessage(UINT msg)
	{
		return msg == WM_MOUSEMOVE
		    || msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_LBUTTONDBLCLK
		    || msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP || msg == WM_RBUTTONDBLCLK
		    || msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP || msg == WM_MBUTTONDBLCLK;
	}

	LRESULT CALLBACK GameWndProc(HWND h, UINT msg, WPARAM w, LPARAM l)
	{
		// NO MOUSE REMAPPING. It was added on the theory that insetting the image
		// would throw clicks off by the inset, and it is removed because it was
		// built on the letterbox rectangle -- the one measurement that
		// demonstrably disagrees with the screen.
		//
		// What it actually did: compress X by 2048/1536 and shift it by 256,
		// while leaving Y an identity. So the pointer could never drive the game
		// to its own right edge (at screen x=800 the game was told 725), and
		// everything left of x=256 collapsed onto game-x 0. That is exactly the
		// reported asymmetry -- perfect on the left, stopping early on the right,
		// with vertical almost right -- and it was OUR distortion, not the
		// game's.
		//
		// Without it the game receives raw client coordinates, which is what the
		// 1:1 hypothesis needs in order to be tested at all.
		if ((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) && w == VK_F11)
		{
			// Swallowed, never forwarded: F11 is not in the panel's key table
			// either, so the game has never had a use for it.
			D2GamePanel_SetGameFullscreen(0);
			return 0;
		}
		return g_hookedUnicode ? CallWindowProcW(g_origProc, h, msg, w, l)
		                       : CallWindowProcA(g_origProc, h, msg, w, l);
	}

	// Caller holds g_lock.
	void HookGameWindow(HWND h)
	{
		if (g_hookedHwnd == h)
			return;
		g_hookedUnicode = IsWindowUnicode(h) != FALSE;
		g_origProc = (WNDPROC)(g_hookedUnicode
			? SetWindowLongPtrW(h, GWLP_WNDPROC, (LONG_PTR)GameWndProc)
			: SetWindowLongPtrA(h, GWLP_WNDPROC, (LONG_PTR)GameWndProc));
		g_hookedHwnd = g_origProc ? h : nullptr;
	}

	// Caller holds g_lock. Leaving our proc installed past teardown would send
	// F11 into freed code the first time it was pressed.
	void UnhookGameWindow()
	{
		if (!g_hookedHwnd || !g_origProc)
			return;
		if (IsWindow(g_hookedHwnd))
		{
			if (g_hookedUnicode)
				SetWindowLongPtrW(g_hookedHwnd, GWLP_WNDPROC, (LONG_PTR)g_origProc);
			else
				SetWindowLongPtrA(g_hookedHwnd, GWLP_WNDPROC, (LONG_PTR)g_origProc);
		}
		g_hookedHwnd = nullptr;
		g_origProc = nullptr;
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
	// Installed here rather than at startup because this is the first point at
	// which the window is known to exist. Idempotent.
	HookGameWindow(h);

	// Same coordinate space as SaveOnce, for the whole of the apply below --
	// the restore path replays g_rect and must read it back as it was written.
	UnawareScope unaware;

	// Always come back to a known state before applying the next mode, so
	// switching between them cannot leave a layered style stacked on an
	// off-screen position, or a borderless fullscreen style on a hidden window.
	SetWindowLongA(h, GWL_EXSTYLE, g_exStyle);
	SetWindowLongA(h, GWL_STYLE, g_style);
	ShowWindow(h, SW_SHOW);
	// SIZE as well as position. This used to pass SWP_NOSIZE, which was right
	// while every mode only moved the window or restyled it -- FULLSCREEN is the
	// first one that resizes it, and without this the window keeps its
	// monitor-filling size forever after the first use. Measured: back at mode 3
	// the presenting blit was still 2042x1123 instead of the 800x600 it started
	// at, so "restore" was handing back a window the size of the screen.
	SetWindowPos(h, nullptr, g_rect.left, g_rect.top,
	             g_rect.right - g_rect.left, g_rect.bottom - g_rect.top,
	             SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

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
	case 4:
	{
		// Borderless, filling the monitor the window is already on, and in
		// front. The debugger yields topmost first: it is always-on-top, so
		// bringing the game forward while it still held that status would put
		// the game in front of everything EXCEPT the one window covering it.
		D2Host_SetTopmost(0);

		// The monitor rect is taken in the same UNAWARE space as everything else
		// here (the scope is still open). Mixing in a per-monitor-aware rect
		// would overshoot by the scale factor and hang the window off the
		// bottom-right of the screen.
		MONITORINFO mi{};
		mi.cbSize = sizeof(mi);
		HMONITOR mh = MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST);
		if (!GetMonitorInfoA(mh, &mi))
		{
			// No monitor info means no rect to fill. Fail the mode rather than
			// guessing at a size -- a wrong one here is a window covering the
			// screen at the wrong dimensions with its title bar already gone.
			D2Host_SetTopmost(1);
			mode = g_mode.load(std::memory_order_relaxed);
			break;
		}
		SetWindowLongA(h, GWL_STYLE,
		               g_style & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX
		                           | WS_MAXIMIZEBOX | WS_SYSMENU));
		SetWindowPos(h, HWND_TOP,
		             mi.rcMonitor.left, mi.rcMonitor.top,
		             mi.rcMonitor.right - mi.rcMonitor.left,
		             mi.rcMonitor.bottom - mi.rcMonitor.top,
		             SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		SetForegroundWindow(h);
		g_fsHwnd.store(h, std::memory_order_relaxed);
		break;
	}
	default:
		mode = 0;
		break;
	}
	// Reclaim topmost whenever we are NOT the fullscreen mode. Unconditional on
	// purpose: leaving fullscreen by any route -- F11, the HTTP endpoint, or
	// teardown -- has to put the debugger back in front, and routing that
	// through one place is what stops a route being missed.
	if (mode != 4)
	{
		D2Host_SetTopmost(1);
		// And give the pointer back. Leaving a clip applied to a window that is
		// no longer full screen is the "clip nobody in the panel owns" failure
		// this codebase has already paid for once.
		g_fsHwnd.store(nullptr, std::memory_order_relaxed);
		D2VInput_ClipCursorReal(nullptr);
	}
	g_mode.store(mode, std::memory_order_relaxed);
	return 1;
}

// Confine the pointer to the full-screen game.
//
// WE HAVE TO DO THIS -- the game will not. Measured on a four-monitor desktop:
// with the window borderless across the primary, the applied clip was the whole
// virtual desktop (9216x2880), input parked or not. D2 only confines the cursor
// when IT believes it is full screen, and it has no idea we resized its window,
// so as far as it is concerned it is still windowed and it never calls
// ClipCursor at all. Nothing was escaping a bad clip; there was no clip.
//
// The rect and the clip are taken in the SAME unaware scope on purpose. That
// pairing is the whole trick: ClipCursor interprets its rectangle in the
// calling thread's DPI space, and this is re-asserted from the debugger's
// per-monitor-aware render thread, so a rect fetched outside the scope would be
// 1.25x off -- exactly the physical-vs-logical mismatch that still misplaces
// the panel's own Capture rect.
extern "C" int D2GameWindow_ApplyFullscreenClip()
{
	HWND h = (HWND)g_fsHwnd.load(std::memory_order_relaxed);
	if (!h)
		return 0;
	UnawareScope unaware;
	RECT wr{};
	if (!GetWindowRect(h, &wr))
		return 0;

	// EXPERIMENT: confine to the game's RENDER RESOLUTION at the window origin,
	// 1:1, rather than to the window.
	//
	// The hypothesis is that D2 treats a client coordinate AS a render
	// coordinate, with no scaling -- so on a 2048x1152 window only the top-left
	// 1068x600 is interactive and everything right of that is off the game's own
	// edge. The ratios line up exactly, which is what makes it worth testing:
	// 1068/2048 = 0.521 and 600/1152 = 0.521, the same factor on both axes, so
	// this rect is precisely the un-scaled corner. It also matches the reported
	// symptom -- the pointer running off to the RIGHT first -- because the
	// horizontal overshoot is the larger one at the 800x600 menu (2.56x against
	// 1.92x vertically).
	//
	// Anchored at the window origin, NOT centred: a 1:1 mapping starts at the
	// client's own (0,0), so centring it would test a different theory.
	//
	// If the pointer now stops exactly at the edge of what the game responds to,
	// the hypothesis holds and this becomes the real fix -- driven from the live
	// source size rather than a constant. If it stops somewhere arbitrary, the
	// hypothesis is dead and this reverts to the window rect.
	// Driven from the LIVE render size, not a constant: D2 rasterises 800x600 at
	// menus and 1068x600 in-world, and under this hypothesis the interactive
	// area is whatever it is currently rendering. A fixed 1068 would be too wide
	// at the menu by exactly the amount that matters. Falls back to the play
	// resolution if no blit has been seen yet.
	int kRenderW = 1068, kRenderH = 600;
	D2Probe_SourceSize(&kRenderW, &kRenderH);
	RECT ir{ wr.left, wr.top, wr.left + kRenderW, wr.top + kRenderH };
	if (ir.right  > wr.right)  ir.right  = wr.right;
	if (ir.bottom > wr.bottom) ir.bottom = wr.bottom;
	if (ir.right > ir.left && ir.bottom > ir.top)
		return D2VInput_ClipCursorReal(&ir);
	return D2VInput_ClipCursorReal(&wr);
}

// Is the window deliberately out of sight? Modes 1-3 conceal it; mode 4 is the
// one that shows it. Callers that used to test `mode != 0` mean THIS.
extern "C" int D2GameWindow_IsConcealed()
{
	const int m = g_mode.load(std::memory_order_relaxed);
	return (m >= 1 && m <= 3) ? 1 : 0;
}

// Read from the presenting blit hook (D2Debugger.probe.cpp), which letterboxes
// only while this is true. Cheap by construction -- one relaxed load on a path
// that runs 25 times a second.
extern "C" int D2GameWindow_IsFullscreen()
{
	return g_mode.load(std::memory_order_relaxed) == 4 ? 1 : 0;
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
	// AFTER the restore, which needs the window intact, and under the lock the
	// restore has already released. Our proc must not outlive the DLL: the first
	// F11 afterwards would call into unloaded code.
	std::lock_guard<std::mutex> guard(g_lock);
	UnhookGameWindow();
}
