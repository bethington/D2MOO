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
		if (!g_enabled.load(std::memory_order_relaxed))
			return real_SetCursorPos(x, y);
		// Swallowed: report success so the caller's logic proceeds normally,
		// but never move the operator's pointer.
		return TRUE;
	}

	BOOL WINAPI Hooked_ClipCursor(const RECT* r)
	{
		if (!g_enabled.load(std::memory_order_relaxed))
			return real_ClipCursor(r);
		return TRUE;    // never confine the real pointer while virtual
	}

	SHORT WINAPI Hooked_GetAsyncKeyState(int vk)
	{
		g_nGetAsyncKey.fetch_add(1, std::memory_order_relaxed);
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
extern "C" void D2VInput_SetEnabled(int on)
{
	if (on && !g_enabled.load())
	{
		POINT p{ 0, 0 };
		if (real_GetCursorPos && real_GetCursorPos(&p))
		{
			g_vx.store(p.x); g_vy.store(p.y);
		}
	}
	if (!on)
	{
		// Release every synthetic key on the way out, or a key left "down"
		// would stick for the rest of the session with nothing holding it.
		for (int i = 0; i < 256; ++i)
		{
			g_down[i].store(false, std::memory_order_relaxed);
			g_pressedEdge[i].store(false, std::memory_order_relaxed);
		}
	}
	g_enabled.store(on != 0);
	VLog(on ? "virtual mode ON" : "virtual mode OFF");
}

extern "C" int D2VInput_IsEnabled() { return g_enabled.load() ? 1 : 0; }

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
extern "C" int D2VInput_PostKey(int vk, int down)
{
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
extern "C" int D2VInput_ClipCursorReal(const void* rect)
{
	if (!real_ClipCursor)
		return 0;
	return real_ClipCursor((const RECT*)rect) ? 1 : 0;
}

extern "C" int D2VInput_PostMouseButton(int button, int down, int clientX, int clientY)
{
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

	// Position first: D2 acts on the click at the cursor position it last saw,
	// so a button arriving before the move would be applied at the OLD spot.
	PostMessageA(h, WM_MOUSEMOVE, wp, lp);
	const UINT msg = right ? (down ? WM_RBUTTONDOWN : WM_RBUTTONUP)
	                       : (down ? WM_LBUTTONDOWN : WM_LBUTTONUP);
	PostMessageA(h, msg, wp, lp);
	return 1;
}

extern "C" int D2VInput_PostMouseMove(int clientX, int clientY)
{
	HWND h = FindWindowA(nullptr, "Diablo II");
	if (!h)
		return 0;
	const LPARAM lp = (LPARAM)((clientY << 16) | (clientX & 0xFFFF));
	WPARAM wp = 0;
	if (g_down[VK_LBUTTON].load(std::memory_order_relaxed)) wp |= MK_LBUTTON;
	if (g_down[VK_RBUTTON].load(std::memory_order_relaxed)) wp |= MK_RBUTTON;
	PostMessageA(h, WM_MOUSEMOVE, wp, lp);
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
