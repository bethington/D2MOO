// D2Debugger.vinput.cpp -- VIRTUAL INPUT: drive the game without touching the
// operator's physical mouse or keyboard.
//
// WHY THIS EXISTS
// ---------------
// The existing driver (D2Asset_HoverXY) parks the REAL OS cursor:
//
//     GetCursorPos(&cur); ... cur.x += stepX; SetCursorPos(cur.x, cur.y);
//
// and re-warps the pointer to the game window's centre whenever it drifts
// outside. That works, and it is deliberately feedback-driven (it re-estimates
// screen-px-per-game-px from what each move actually did, so it needs no DPI or
// window constants). But it makes automation ATTENDED-ONLY: the pointer jumps
// into the game and walks to its target, so it fights the operator for the
// mouse and cannot run while the machine is in use.
//
// The reachable-gameplay problem this unblocks is concrete: 71 of the 84
// promotable shadow dispatchers sit at ZERO hits because nothing plays the
// content that calls them, and BATTLETESTED promotion needs >=1000 hits with
// >=20 distinct inputs of real gameplay.
//
// HOW THE GAME ACTUALLY READS INPUT (measured from the PE imports, 2026-07-31)
// ---------------------------------------------------------------------------
//     ddraw.dll (cnc-ddraw) : GetCursorPos SetCursorPos ClipCursor
//                             GetAsyncKeyState PeekMessageA SetCapture
//     D2Client.dll          : GetAsyncKeyState GetKeyState GetMessageA
//                             PeekMessageA SetCursorPos
//     D2Win.dll             : GetKeyState GetMessageA PeekMessageA SetCursorPos
//
// That import list is what the game COULD use; what it actually does was
// settled by counters and then by outcome:
//
//   * GetCursorPos is never called (0 hits in-world) -- cnc-ddraw runs
//     handlemouse=true and takes POSITION from the MESSAGE QUEUE.
//   * GetAsyncKeyState is hammered (1.27M hits), but synthesising it is NOT
//     sufficient for buttons: with the async state set and no message posted,
//     the cursor tracked perfectly and clicks did nothing at all.
//   * KEYS behave like buttons for the same reason -- the message queue is the
//     authority, and the polled state is a secondary view of it.
//
// So everything is posted as a real message AND mirrored into the polled state,
// so whichever path a given call site reads, both agree.
//
// WHAT IS HOOKED, AND WHY EACH
//   GetCursorPos     -- report the virtual position instead of the real one.
//   GetAsyncKeyState -- report synthetic button/key state (VK_LBUTTON etc).
//   GetKeyState      -- same, for the D2Client/D2Win path.
//   SetCursorPos     -- SWALLOWED while virtual. cnc-ddraw and D2Client both
//                       call it; unhooked, the game would yank the real pointer
//                       back and fight our virtual position.
//   ClipCursor       -- SWALLOWED while virtual. cnc-ddraw confines the cursor
//                       to the game window; that confinement is exactly what
//                       makes the real-cursor approach steal the pointer.
//
// FAIL-SAFE BY CONSTRUCTION: every hook is a pass-through unless virtual mode
// is explicitly enabled. Disabled is bit-identical to not being installed, so
// this cannot perturb ordinary play or the existing HoverXY path.
#include <Windows.h>
#include <detours.h>
#include <atomic>
#include <cstdio>

namespace
{
	// Window visibility mode, owned by D2Debugger.gamewindow.cpp: 0 = the real
	// window is shown, non-zero = hidden in one of several ways.
	extern "C" int D2GameWindow_Mode();

	using GetCursorPosFn = BOOL(WINAPI*)(LPPOINT);
	using SetCursorPosFn = BOOL(WINAPI*)(int, int);
	using ClipCursorFn = BOOL(WINAPI*)(const RECT*);
	using GetAsyncKeyStateFn = SHORT(WINAPI*)(int);
	using GetKeyStateFn = SHORT(WINAPI*)(int);

	GetCursorPosFn     real_GetCursorPos = nullptr;
	SetCursorPosFn     real_SetCursorPos = nullptr;
	ClipCursorFn       real_ClipCursor = nullptr;
	GetAsyncKeyStateFn real_GetAsyncKeyState = nullptr;
	GetKeyStateFn      real_GetKeyState = nullptr;

	// The panel is driving: gates the Post* senders, and swallows the GAME's
	// own SetCursorPos/ClipCursor so it can neither fling nor trap the
	// operator's pointer. True in virtual AND physical mode -- protecting the
	// pointer has nothing to do with where key state comes from.
	std::atomic<bool> g_armed{ false };
	// The reporting hooks LIE: synthetic key and cursor state. Virtual only.
	std::atomic<bool> g_enabled{ false };
	std::atomic<bool> g_installed{ false };
	std::atomic<long> g_vx{ 0 };            // virtual cursor, SCREEN coordinates
	std::atomic<long> g_vy{ 0 };

	// 256 virtual-key slots. `down` is the live state; `pressedEdge` records a
	// press that has not yet been observed, so a click that begins and ends
	// between two of the game's polls is still seen.
	//
	// D2 polls GetAsyncKeyState once per frame. A naive down-then-up inside one
	// frame is invisible -- the classic synthetic-input bug. The edge latch is
	// consumed by the first GetAsyncKeyState that reports it, which is exactly
	// the semantics of that API's low bit ("pressed since last call").
	std::atomic<bool> g_down[256];
	std::atomic<bool> g_pressedEdge[256];

	// Instrumentation: does the game actually POLL these, or does it derive its
	// mouse from window messages? cnc-ddraw runs with handlemouse=true and
	// imports PeekMessageA, so the answer decides whether lying about
	// GetCursorPos can work at all. Measure rather than assume.
	std::atomic<unsigned long> g_nGetCursorPos{ 0 };
	std::atomic<unsigned long> g_nGetAsyncKey{ 0 };
	std::atomic<unsigned long> g_nGetKeyState{ 0 };
	// Per-VK query histogram, so which key codes the game actually polls is a
	// reading rather than an assumption. Two arrays: one per hook.
	std::atomic<unsigned long> g_vkAsync[256] = {};
	std::atomic<unsigned long> g_vkState[256] = {};
	// Our render thread, set every frame from the panel. A poll from any OTHER
	// thread is the game; polls from this thread are ImGui's own backend
	// (which also reads GetKeyState on the modifiers) and are excluded, so the
	// histogram shows what D2 ALONE reads.
	std::atomic<unsigned long> g_renderTid{ 0 };

	void VLog(const char* msg)
	{
		FILE* f = nullptr;
		if (fopen_s(&f, "C:\\Users\\benam\\source\\cpp\\D2MOO\\conformance\\behavioral\\overlay_gl_log.txt", "a") == 0 && f)
		{
			fprintf(f, "[vinput] %s\n", msg);
			fclose(f);
		}
	}

	BOOL WINAPI Hooked_GetCursorPos(LPPOINT p)
	{
		g_nGetCursorPos.fetch_add(1, std::memory_order_relaxed);
		if (!g_enabled.load(std::memory_order_relaxed) || !p)
			return real_GetCursorPos(p);
		p->x = (LONG)g_vx.load(std::memory_order_relaxed);
		p->y = (LONG)g_vy.load(std::memory_order_relaxed);
		return TRUE;
	}

	BOOL WINAPI Hooked_SetCursorPos(int x, int y)
	{
		if (!g_armed.load(std::memory_order_relaxed))
			return real_SetCursorPos(x, y);
		// Swallowed: report success so the caller's logic proceeds normally,
		// but never move the operator's pointer.
		return TRUE;
	}

	BOOL WINAPI Hooked_ClipCursor(const RECT* r)
	{
		// Releasing a clip is ALWAYS allowed to pass through -- refusing to let
		// anyone un-confine the pointer is how it gets stuck.
		if (!r)
			return real_ClipCursor(nullptr);

		// Swallow the game's clip while the panel is driving...
		if (g_armed.load(std::memory_order_relaxed))
			return TRUE;

		// ...and ALSO whenever the real window is hidden, armed or not. A hidden
		// window confining the pointer is never correct: the rectangle is
		// somewhere the operator cannot see, clicking out of it is impossible,
		// and the panel's Ctrl+Alt break-out does not help because the panel
		// never applied that clip. This cost two sessions -- the second one
		// ended with the game being killed to get the mouse back, with Capture
		// unticked the whole time, which is exactly the signature of a clip
		// nobody in the panel owns.
		if (D2GameWindow_Mode() != 0)
			return TRUE;

		return real_ClipCursor(r);
	}

	SHORT WINAPI Hooked_GetAsyncKeyState(int vk)
	{
		g_nGetAsyncKey.fetch_add(1, std::memory_order_relaxed);
		if (vk >= 0 && vk <= 255
		    && GetCurrentThreadId() != g_renderTid.load(std::memory_order_relaxed))
			g_vkAsync[vk].fetch_add(1, std::memory_order_relaxed);
		if (!g_enabled.load(std::memory_order_relaxed) || vk < 0 || vk > 255)
			return real_GetAsyncKeyState(vk);
		SHORT s = 0;
		if (g_down[vk].load(std::memory_order_relaxed))
			s |= (SHORT)0x8000;                       // currently down
		// Low bit = "pressed since the previous call"; consume the latch so it
		// reports exactly once, matching the real API.
		if (g_pressedEdge[vk].exchange(false, std::memory_order_relaxed))
			s |= (SHORT)0x0001;
		return s;
	}

	SHORT WINAPI Hooked_GetKeyState(int vk)
	{
		g_nGetKeyState.fetch_add(1, std::memory_order_relaxed);
		if (vk >= 0 && vk <= 255
		    && GetCurrentThreadId() != g_renderTid.load(std::memory_order_relaxed))
			g_vkState[vk].fetch_add(1, std::memory_order_relaxed);
		if (!g_enabled.load(std::memory_order_relaxed) || vk < 0 || vk > 255)
			return real_GetKeyState(vk);
		return g_down[vk].load(std::memory_order_relaxed) ? (SHORT)0x8000 : (SHORT)0;
	}
}

// ---------------------------------------------------------------- public ----

extern "C" void D2VInput_Install()
{
	if (g_installed.exchange(true))
		return;
	HMODULE u32 = GetModuleHandleW(L"user32.dll");
	if (!u32)
	{
		VLog("install FAILED: user32 not loaded");
		return;
	}
	real_GetCursorPos = (GetCursorPosFn)GetProcAddress(u32, "GetCursorPos");
	real_SetCursorPos = (SetCursorPosFn)GetProcAddress(u32, "SetCursorPos");
	real_ClipCursor = (ClipCursorFn)GetProcAddress(u32, "ClipCursor");
	real_GetAsyncKeyState = (GetAsyncKeyStateFn)GetProcAddress(u32, "GetAsyncKeyState");
	real_GetKeyState = (GetKeyStateFn)GetProcAddress(u32, "GetKeyState");

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	if (real_GetCursorPos)     DetourAttach(&(PVOID&)real_GetCursorPos, (PVOID)Hooked_GetCursorPos);
	if (real_SetCursorPos)     DetourAttach(&(PVOID&)real_SetCursorPos, (PVOID)Hooked_SetCursorPos);
	if (real_ClipCursor)       DetourAttach(&(PVOID&)real_ClipCursor, (PVOID)Hooked_ClipCursor);
	if (real_GetAsyncKeyState) DetourAttach(&(PVOID&)real_GetAsyncKeyState, (PVOID)Hooked_GetAsyncKeyState);
	if (real_GetKeyState)      DetourAttach(&(PVOID&)real_GetKeyState, (PVOID)Hooked_GetKeyState);
	const LONG err = DetourTransactionCommit();

	char buf[240];
	_snprintf_s(buf, sizeof(buf), _TRUNCATE,
		"install commit=%ld GetCursorPos=%p SetCursorPos=%p ClipCursor=%p "
		"GetAsyncKeyState=%p GetKeyState=%p",
		err, (void*)real_GetCursorPos, (void*)real_SetCursorPos,
		(void*)real_ClipCursor, (void*)real_GetAsyncKeyState, (void*)real_GetKeyState);
	VLog(buf);
}

// Enable/disable virtual mode. On ENABLE the virtual cursor is seeded from the
// real one, so the game sees no discontinuity at the moment of the switch.
// 0 = OFF       nothing is posted, every hook passes through.
// 1 = VIRTUAL   posted input, and the hooks report synthetic state.
// 2 = PHYSICAL  posted input, but the hooks report the REAL device state.
//
// Physical still POSTS, because the real game window is hidden -- without the
// posted messages there would be no way to play at all. What changes is only
// the ~3.5 polled keys per frame D2 reads through GetAsyncKeyState (measured
// live: 364 calls across 104 frames), which is where the held modifiers live.
extern "C" void D2VInput_SetMode(int mode)
{
	const bool virt = (mode == 1);
	if (virt && !g_enabled.load())
	{
		POINT p{ 0, 0 };
		if (real_GetCursorPos && real_GetCursorPos(&p))
		{
			g_vx.store(p.x); g_vy.store(p.y);
		}
	}
	if (!virt)
	{
		// Release every synthetic key on the way out, or a key left "down"
		// would stick for the rest of the session with nothing holding it.
		// Also on virtual -> physical: the synthetic state stops being read,
		// and a stale key left set would resurrect on the way back.
		for (int i = 0; i < 256; ++i)
		{
			g_down[i].store(false, std::memory_order_relaxed);
			g_pressedEdge[i].store(false, std::memory_order_relaxed);
		}
	}
	// Order matters: arm before enabling, disarm after disabling, so no hook
	// can observe the pair half-applied.
	if (mode != 0)
		g_armed.store(true);
	g_enabled.store(virt);
	if (mode == 0)
		g_armed.store(false);
	VLog(mode == 0 ? "input mode OFF"
	   : mode == 1 ? "input mode VIRTUAL"
	               : "input mode PHYSICAL");
}

extern "C" int D2VInput_Mode()
{
	if (!g_armed.load()) return 0;
	return g_enabled.load() ? 1 : 2;
}

// Unchanged meaning for every existing caller (the oracle's POST /input/mode):
// on = virtual, off = fully off.
extern "C" void D2VInput_SetEnabled(int on)
{
	D2VInput_SetMode(on ? 1 : 0);
}

extern "C" int D2VInput_IsEnabled() { return g_enabled.load() ? 1 : 0; }

// The REAL key state, straight past our own hook.
//
// Anything asking the ordinary GetAsyncKeyState from inside this process gets
// the SYNTHETIC view while the virtual layer is armed -- what we last told the
// game, not what the operator is physically holding. The cursor break-out needs
// the physical truth, and needs it precisely when things have gone wrong enough
// that ImGui may not be seeing keys at all.
extern "C" int D2VInput_RealKeyDown(int vk)
{
	if (!real_GetAsyncKeyState)
		return 0;
	return (real_GetAsyncKeyState(vk) & 0x8000) ? 1 : 0;
}

extern "C" void D2VInput_SetScreenPos(int x, int y)
{
	g_vx.store(x); g_vy.store(y);
}

extern "C" void D2VInput_GetScreenPos(int* x, int* y)
{
	if (x) *x = (int)g_vx.load();
	if (y) *y = (int)g_vy.load();
}

// Hold or release a virtual key / mouse button. `down` latches the press edge
// so a same-frame press+release is still observed exactly once.
extern "C" void D2VInput_SetKey(int vk, int down)
{
	if (vk < 0 || vk > 255)
		return;
	const bool was = g_down[vk].load(std::memory_order_relaxed);
	g_down[vk].store(down != 0, std::memory_order_relaxed);
	if (down && !was)
		g_pressedEdge[vk].store(true, std::memory_order_relaxed);
}

// Copy the per-VK query counts out. buf must hold 256 entries per array.
extern "C" void D2VInput_KeyHist(unsigned long* async256, unsigned long* state256)
{
	for (int i = 0; i < 256; ++i)
	{
		if (async256) async256[i] = g_vkAsync[i].load(std::memory_order_relaxed);
		if (state256) state256[i] = g_vkState[i].load(std::memory_order_relaxed);
	}
}

extern "C" void D2VInput_MarkRenderThread()
{
	g_renderTid.store(GetCurrentThreadId(), std::memory_order_relaxed);
}

extern "C" void D2VInput_KeyHistReset()
{
	for (int i = 0; i < 256; ++i)
	{
		g_vkAsync[i].store(0, std::memory_order_relaxed);
		g_vkState[i].store(0, std::memory_order_relaxed);
	}
}

extern "C" void D2VInput_GetCounters(unsigned long* cursor, unsigned long* asyncKey,
                                     unsigned long* keyState)
{
	if (cursor)   *cursor = g_nGetCursorPos.load(std::memory_order_relaxed);
	if (asyncKey) *asyncKey = g_nGetAsyncKey.load(std::memory_order_relaxed);
	if (keyState) *keyState = g_nGetKeyState.load(std::memory_order_relaxed);
}

// Post a WM_MOUSEMOVE to the game window in CLIENT coordinates.
//
// cnc-ddraw runs with handlemouse=true and imports PeekMessageA; if the game
// derives its mouse from the message queue rather than from polling
// GetCursorPos, lying about the cursor cannot move it and this is the lever
// that can. In-process PostMessage is not subject to UIPI, which is what makes
// this viable at all -- the same call from an operator shell would be dropped
// silently because the game runs elevated.
// Post a mouse BUTTON transition, in game client coordinates.
//
// The GetAsyncKeyState latch below is necessary but NOT sufficient: position
// reaches D2 through the MESSAGE QUEUE (cnc-ddraw runs handlemouse=true and
// D2Client pumps GetMessageA/PeekMessageA), and clicks travel the same way. A
// synthetic button that only sets the async-key state moves nothing -- observed
// live: the cursor tracked the panel perfectly while clicks did nothing at all.
//
// Sends the DOWN/UP pair as real messages, and keeps the async state in sync so
// whichever path a given call site reads agrees with the other.
// Post a KEY transition as a real WM_KEYDOWN/WM_KEYUP.
//
// Same lesson as the mouse buttons: setting the GetAsyncKeyState view alone
// leaves the message-queue consumers (D2Win's dialog/menu handling, chat, the
// UI toggles) completely unaware. The polled state is mirrored too, because the
// skill/movement paths do read it.
//
// lParam is built properly rather than passed as 0: D2Win inspects the scan
// code and the transition/previous-state bits, and a malformed lParam is
// accepted silently and then ignored, which is indistinguishable from the key
// never arriving.
// Post a message with COORDINATES under the target window's own DPI context.
//
// The caller's thread context is part of the message: posting WM_MOUSEMOVE from
// a PER_MONITOR_AWARE_V2 thread to this DPI-unaware game window makes Windows
// rescale the lParam by 96/dpi -- measured x0.800 exactly on a 125% monitor,
// which put every panel-routed cursor position 20% short while the identical
// call from an unaware HTTP thread arrived 1:1. Switching to the window's own
// context for the duration makes the post mean the same thing from any thread;
// on threads already matching (or on pre-1607 Windows, which has no message
// virtualization at all) it is a no-op.
static BOOL PostInWindowContext(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
	using GetWinCtxFn = DPI_AWARENESS_CONTEXT(WINAPI*)(HWND);
	using SetThreadCtxFn = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
	static GetWinCtxFn getWinCtx = nullptr;
	static SetThreadCtxFn setThreadCtx = nullptr;
	static bool resolved = false;
	if (!resolved)
	{
		resolved = true;
		if (HMODULE u32 = GetModuleHandleW(L"user32.dll"))
		{
			getWinCtx = (GetWinCtxFn)GetProcAddress(u32, "GetWindowDpiAwarenessContext");
			setThreadCtx = (SetThreadCtxFn)GetProcAddress(u32, "SetThreadDpiAwarenessContext");
		}
	}
	if (!getWinCtx || !setThreadCtx)
		return PostMessageA(h, msg, wp, lp);

	DPI_AWARENESS_CONTEXT prev = setThreadCtx(getWinCtx(h));
	const BOOL ok = PostMessageA(h, msg, wp, lp);
	if (prev)
		setThreadCtx(prev);
	return ok;
}

// Post Alt as a SYSTEM key -- WM_SYSKEYDOWN / WM_SYSKEYUP(VK_MENU) -- because
// that is what physically holding Alt over a window generates, and D2 reads the
// show-items modifier from that message rather than by polling (measured: the
// game thread never queries VK_MENU via GetKeyState or GetAsyncKeyState).
//
// The lParam mirrors a real Alt event: scan 0x38, the context bit (29) set while
// Alt is the active modifier, and the prev-state/transition bits (30/31) on the
// up. WM_KEYDOWN would be the wrong message type for a system key.
extern "C" int D2VInput_PostAltHold(int down)
{
	if (!g_armed.load(std::memory_order_relaxed))
		return 0;
	HWND h = FindWindowA(nullptr, "Diablo II");
	if (!h)
		return 0;

	// Keep the polled/MK view consistent too, harmless if nothing reads it.
	D2VInput_SetKey(VK_MENU, down);

	const UINT scan = MapVirtualKeyA(VK_MENU, MAPVK_VK_TO_VSC);   // 0x38
	LPARAM lp = 1;                                   // repeat count
	lp |= (LPARAM)(scan & 0xFF) << 16;
	if (down)
	{
		lp |= (LPARAM)1 << 29;                       // context: Alt is down
		PostMessageA(h, WM_SYSKEYDOWN, (WPARAM)VK_MENU, lp);
	}
	else
	{
		lp |= ((LPARAM)1 << 30) | ((LPARAM)1 << 31); // prev-down + transition up
		PostMessageA(h, WM_SYSKEYUP, (WPARAM)VK_MENU, lp);
	}
	return 1;
}

extern "C" int D2VInput_PostKey(int vk, int down)
{
	// Disarmed means DISARMED. These post real window messages, and once did it
	// regardless of the flag -- so turning input off still drove the game, which
	// is exactly why the control looked like a no-op. Keyed on ARMED, not
	// virtual: physical mode posts too, it just does not lie about key state.
	if (!g_armed.load(std::memory_order_relaxed))
		return 0;
	if (vk < 0 || vk > 255)
		return 0;
	HWND h = FindWindowA(nullptr, "Diablo II");
	if (!h)
		return 0;

	const bool wasDown = g_down[vk].load(std::memory_order_relaxed);
	D2VInput_SetKey(vk, down);

	const UINT scan = MapVirtualKeyA((UINT)vk, MAPVK_VK_TO_VSC);
	// Extended keys carry bit 24; without it the arrows/nav cluster decode as
	// their numpad twins.
	bool ext = false;
	switch (vk)
	{
	case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
	case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
	case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
	case VK_RCONTROL: case VK_RMENU:
		ext = true;
		break;
	default:
		break;
	}

	LPARAM lp = 1;                                  // repeat count
	lp |= (LPARAM)(scan & 0xFF) << 16;
	if (ext)      lp |= (LPARAM)1 << 24;
	if (!down)    lp |= ((LPARAM)1 << 30) | ((LPARAM)1 << 31);   // prev-down + transition
	else if (wasDown) lp |= (LPARAM)1 << 30;                     // auto-repeat

	PostMessageA(h, down ? WM_KEYDOWN : WM_KEYUP, (WPARAM)vk, lp);
	return 1;
}

// Apply a REAL cursor clip, bypassing our own hook.
//
// Hooked_ClipCursor swallows the GAME's clip calls on purpose -- unhooked, the
// game confines the pointer and fights our virtual position. But the panel's
// cursor capture needs a genuine clip, so it calls the trampoline directly:
// ours goes through, the game's still does not.
//
// Windows drops the clip whenever the window loses activation, so the caller is
// expected to re-apply it rather than set it once.
// Last rectangle handed to ClipCursor, verbatim. Reported by /input/state
// alongside what Windows actually applied.
std::atomic<long> g_reqL{ 0 }, g_reqT{ 0 }, g_reqR{ 0 }, g_reqB{ 0 };

extern "C" int D2VInput_ClipCursorReal(const void* rect)
{
	if (!real_ClipCursor)
		return 0;
	if (const RECT* r = (const RECT*)rect)
	{
		g_reqL.store(r->left,   std::memory_order_relaxed);
		g_reqT.store(r->top,    std::memory_order_relaxed);
		g_reqR.store(r->right,  std::memory_order_relaxed);
		g_reqB.store(r->bottom, std::memory_order_relaxed);
	}
	else
	{
		g_reqL.store(0, std::memory_order_relaxed);
		g_reqT.store(0, std::memory_order_relaxed);
		g_reqR.store(0, std::memory_order_relaxed);
		g_reqB.store(0, std::memory_order_relaxed);
	}
	// Straight through. The logical->physical conversion belongs at the call
	// site, where the window (and therefore the scale) is known -- see
	// WindowScale in D2Debugger.gamepanel.cpp.
	//
	// A measure-the-transform-and-compensate pass used to live here. It was
	// built on a wrong model of which API virtualises what, never demonstrably
	// fired, and with a real conversion upstream a second corrective layer could
	// only make the next failure harder to read.
	return real_ClipCursor((const RECT*)rect) ? 1 : 0;
}

extern "C" void D2VInput_LastClipRequest(long* l, long* t, long* r, long* b)
{
	if (l) *l = g_reqL.load(std::memory_order_relaxed);
	if (t) *t = g_reqT.load(std::memory_order_relaxed);
	if (r) *r = g_reqR.load(std::memory_order_relaxed);
	if (b) *b = g_reqB.load(std::memory_order_relaxed);
}

extern "C" int D2VInput_PostMouseButton(int button, int down, int clientX, int clientY)
{
	// Disarmed means DISARMED. These post real window messages, and once did it
	// regardless of the flag -- so turning input off still drove the game, which
	// is exactly why the control looked like a no-op. Keyed on ARMED, not
	// virtual: physical mode posts too, it just does not lie about key state.
	if (!g_armed.load(std::memory_order_relaxed))
		return 0;
	HWND h = FindWindowA(nullptr, "Diablo II");
	if (!h)
		return 0;
	const bool right = (button == VK_RBUTTON);
	const int vk = right ? VK_RBUTTON : VK_LBUTTON;

	// Keep the polled view consistent with the message we are about to post.
	D2VInput_SetKey(vk, down);

	const LPARAM lp = (LPARAM)((clientY << 16) | (clientX & 0xFFFF));
	WPARAM wp = 0;
	if (g_down[VK_LBUTTON].load(std::memory_order_relaxed)) wp |= MK_LBUTTON;
	if (g_down[VK_RBUTTON].load(std::memory_order_relaxed)) wp |= MK_RBUTTON;
	// Held modifiers, so a shift-click (buy/sell a full stack, cast in place) or
	// a ctrl-click (move an item to the stash/cube/belt) is recognised even if
	// the game reads the modifier from the message's wParam rather than polling
	// GetKeyState. There is no MK_ flag for Alt -- ground-item display is polled.
	if (g_down[VK_SHIFT].load(std::memory_order_relaxed))   wp |= MK_SHIFT;
	if (g_down[VK_CONTROL].load(std::memory_order_relaxed)) wp |= MK_CONTROL;

	// Position first: D2 acts on the click at the cursor position it last saw,
	// so a button arriving before the move would be applied at the OLD spot.
	PostInWindowContext(h, WM_MOUSEMOVE, wp, lp);
	const UINT msg = right ? (down ? WM_RBUTTONDOWN : WM_RBUTTONUP)
	                       : (down ? WM_LBUTTONDOWN : WM_LBUTTONUP);
	// Button messages carry client coordinates in lParam, so they are subject
	// to the same cross-DPI-context rescale as WM_MOUSEMOVE -- a click posted
	// from the aware render thread would land 20% up-left of the cursor.
	PostInWindowContext(h, msg, wp, lp);
	return 1;
}

extern "C" int D2VInput_PostMouseMove(int clientX, int clientY)
{
	// Disarmed means DISARMED. These post real window messages, and once did it
	// regardless of the flag -- so turning input off still drove the game, which
	// is exactly why the control looked like a no-op. Keyed on ARMED, not
	// virtual: physical mode posts too, it just does not lie about key state.
	if (!g_armed.load(std::memory_order_relaxed))
		return 0;
	HWND h = FindWindowA(nullptr, "Diablo II");
	if (!h)
		return 0;
	const LPARAM lp = (LPARAM)((clientY << 16) | (clientX & 0xFFFF));
	WPARAM wp = 0;
	if (g_down[VK_LBUTTON].load(std::memory_order_relaxed)) wp |= MK_LBUTTON;
	if (g_down[VK_RBUTTON].load(std::memory_order_relaxed)) wp |= MK_RBUTTON;
	// Held modifiers, so a shift-click (buy/sell a full stack, cast in place) or
	// a ctrl-click (move an item to the stash/cube/belt) is recognised even if
	// the game reads the modifier from the message's wParam rather than polling
	// GetKeyState. There is no MK_ flag for Alt -- ground-item display is polled.
	if (g_down[VK_SHIFT].load(std::memory_order_relaxed))   wp |= MK_SHIFT;
	if (g_down[VK_CONTROL].load(std::memory_order_relaxed)) wp |= MK_CONTROL;
	PostInWindowContext(h, WM_MOUSEMOVE, wp, lp);
	return 1;
}

extern "C" int D2VInput_GetKey(int vk)
{
	if (vk < 0 || vk > 255)
		return 0;
	return g_down[vk].load(std::memory_order_relaxed) ? 1 : 0;
}

// ---------------------------------------------------------- move-to-game ----
//
// MEASURED 2026-07-31, and it overturned the design this file started with.
// Instrumenting the hooks in-game gave:
//
//     GetCursorPos       0 calls        <- never polled
//     GetAsyncKeyState   1,273,833      <- hammered every frame
//     GetKeyState        0 calls
//
// So the game does NOT read its mouse POSITION from GetCursorPos. ddraw.dll
// imports the symbol, but importing is not calling -- cnc-ddraw runs with
// handlemouse=true and takes position from the MESSAGE QUEUE. Lying about the
// cursor could never have moved it, which is exactly what the first attempt
// saw: the game's mouse view sat frozen at 320,240.
//
// BUTTONS and KEYS do come through GetAsyncKeyState, so that half of the
// original design is correct and stays.
//
// The message path then turned out to be 1:1 with no scaling: posting
// WM_MOUSEMOVE at client (100,80) put the game's mouse at exactly (100,80),
// and (900,500) at (900,500). So no feedback loop, no scale estimation and no
// DPI math is needed here -- the game's coordinate space IS the window's
// client space. The loop below exists only to CONFIRM the move landed, which
// costs one readback and turns a silent no-op into a reported failure.
//
// In-process PostMessage is also what makes this usable at all: the game runs
// elevated, so the identical call from an operator shell is dropped by UIPI
// with no error.
extern "C" int D2VInput_MoveToGameXY(int gameX, int gameY, int* outX, int* outY)
{
	uintptr_t d2client = (uintptr_t)GetModuleHandleA("D2Client.dll");
	if (!d2client)
		return -2;
	const uintptr_t kD2ClientBase = 0x6fab0000u;
	volatile int* pMX = (volatile int*)(d2client + (0x6fbcb828u - kD2ClientBase));
	volatile int* pMY = (volatile int*)(d2client + (0x6fbcb824u - kD2ClientBase));

	if (!D2VInput_PostMouseMove(gameX, gameY))
		return -2;                      // game window not found

	// Confirm the game consumed it. It polls its queue once per frame, so a few
	// short waits is plenty; reporting a miss beats returning a false success.
	for (int i = 0; i < 10; ++i)
	{
		Sleep(20);
		const int gx = *pMX, gy = *pMY;
		if (gx == gameX && gy == gameY)
		{
			if (outX) *outX = gx;
			if (outY) *outY = gy;
			g_vx.store(gameX); g_vy.store(gameY);   // keep the virtual view in step
			return 1;
		}
	}
	if (outX) *outX = *pMX;
	if (outY) *outY = *pMY;
	return -3;                          // posted, but the game never took it
}
