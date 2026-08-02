// D2Debugger.probe.cpp -- PRESENT-PATH PROBE: find where the frame actually
// reaches the window.
//
// WHY THIS EXISTS
// ---------------
// D2Debugger.vcapture.cpp needs a hook at present time to read the finished
// frame. Four candidates were hooked, verified attached (DetourTransactionCommit
// returned 0, every symbol resolved) and instrumented with first-fire logging:
//
//     opengl32!wglSwapBuffers        never fired
//     opengl32!wglSwapLayerBuffers   never fired
//     gdi32!SwapBuffers              never fired
//     glide3x!_grBufferSwap@4        never fired
//
// across a full in-world session. So PD2 presents through none of them.
//
// This matters beyond one feature: D2Debugger.overlay.gl.cpp is built entirely
// on the claim that "d2gl/glide3x.dll or cnc-ddraw renderer=opengl -- both
// present via wglSwapBuffers/SwapBuffers". That claim is FALSE here, which is
// very likely why that overlay was written, never wired up (it has no callers),
// and quietly abandoned.
//
// Rather than guess again, this probe measures. Whatever a wrapper does
// internally, pixels must reach a window through GDI, OpenGL, D3D or the
// compositor -- all reachable by NAME, with no COM vtable patching.
//
// WHAT IT RECORDS, AND WHY THAT SHAPE
//   * a per-API call COUNT (an atomic increment -- free at 60 FPS);
//   * the RETURN ADDRESS of the first call, and of the most recent one.
//
// The caller is the real prize: "BitBlt, 60/sec, called from ddraw.dll"
// identifies both the renderer and the exact place to hook, in one session.
//
// The return address is resolved to a MODULE NAME lazily, when the probe is
// queried -- never in the hook. GetModuleHandleEx takes the loader lock, and
// doing that thousands of times a second inside a present path is both a
// performance problem and a deadlock risk.
//
// Counters are always on. They cost one relaxed atomic increment, and they stay
// useful: a PD2 update or a renderer switch changes the present path silently,
// and this reports it instead of costing another session of archaeology.
#include <Windows.h>
#include <detours.h>
#include <atomic>
#include <cstdio>
#include <cstring>

extern "C" int D2Capture_DibReport(char* buf, int cch);
#include <intrin.h>          // _ReturnAddress

// Frame capture reads the DIB the game rasterises into; this file owns the
// detours, so it feeds capture the two facts it needs: which DIBs exist, and
// when one is being presented.
extern "C" void D2Capture_NoteDib(void* bmp, void* bits, int w, int h, int bpp);
extern "C" void D2Capture_OnStretchBlt(void* hdcSrc, int hDst, int hSrc);
// Is the real game window borderless full screen (D2Debugger.gamewindow.cpp)?
// The ONLY condition under which the presenting blit below is rewritten.
extern "C" int D2GameWindow_IsFullscreen();

namespace
{
	struct Probe
	{
		const char* module;
		const char* name;
		std::atomic<unsigned long long> calls{ 0 };
		std::atomic<void*> firstCaller{ nullptr };
		std::atomic<void*> lastCaller{ nullptr };
		void* real = nullptr;
	};

	// Indices into g_probes. Kept explicit so the hook bodies stay trivial.
	enum {
		P_BitBlt, P_StretchBlt, P_StretchDIBits, P_SetDIBitsToDevice, P_PatBlt,
		P_SwapBuffersGdi,
		P_wglSwapBuffers, P_wglSwapLayerBuffers, P_glFlush, P_glFinish, P_glDrawPixels,
		P_DwmFlush, P_CreateDIBSection,
		P_COUNT
	};

	Probe g_probes[P_COUNT];

	// Geometry of the last presenting blit, kept to answer exactly one question:
	// does the game's blit FOLLOW ITS WINDOW, or is it pinned to the render
	// resolution? Whether the real window can be usefully shown full screen
	// rests entirely on that, and it is not answerable by reading D2Gdi -- we do
	// not have its source. The hook already receives all four numbers and threw
	// the widths away.
	//
	// dst != src means GDI is scaling for us and a bigger window means a bigger
	// picture. dst == src regardless of window size means the blit is fixed and
	// no amount of resizing will fill the screen.
	std::atomic<int> g_blitDstW{ 0 }, g_blitDstH{ 0 };
	std::atomic<int> g_blitSrcW{ 0 }, g_blitSrcH{ 0 };

	// ---- full-screen letterboxing -------------------------------------------
	//
	// The game stretches its frame to the WHOLE client rect, so a window sized
	// to the source aspect would not letterbox -- it would just be a smaller
	// window with the desktop around it. The aspect has to be corrected at the
	// blit, which is why this lives here rather than in the window layer.
	//
	// It matters because the source resolution is not one number: D2 renders the
	// menu and character select at 800x600 (4:3) and the world at 1068x600
	// (16:9). Filling a 16:9 screen from the 4:3 source stretches those screens
	// 33% wide. Recomputing per blit tracks the change for free -- there is no
	// state to invalidate when the game enters or leaves a world.
	//
	// The surround is a MIRROR of the image's own edges, softened -- matching
	// what the Game panel already does at menus, so the two ways of watching the
	// game look like the same product rather than two. Plain black bars were
	// tried first and rejected for that reason.
	//
	// Softening is a down-and-back-up scale through HALFTONE, which is as close
	// to a blur as GDI gets. It matters: an unsoftened mirror puts a second,
	// sharp copy of the castle and torches either side of the menu and competes
	// with the thing you are looking at.
	//
	// Redrawn every frame rather than once, so it cannot be left stale by
	// anything else drawing into this DC.
	//
	// WHERE THE IMAGE LANDS, for the mouse. D2 derives the cursor position from
	// its CLIENT RECT -- measured, by pointing at things and having them be hit
	// -- so insetting the image without telling anyone would put every menu
	// click out by the bar width. The rect is published here and the game's
	// wndproc maps incoming mouse messages back through it.
	std::atomic<int> g_lbX{ 0 }, g_lbY{ 0 }, g_lbW{ 0 }, g_lbH{ 0 };
	std::atomic<int> g_lbClientW{ 0 }, g_lbClientH{ 0 };

	inline void LetterboxDest(HDC dc, int& x, int& y, int& w, int& h,
	                          HDC srcDc, int sx, int sy, int srcW, int srcH)
	{
		// Only the plain, unflipped case. A negative width or height on either
		// side means GDI is mirroring for us, and rewriting a mirrored rect is
		// a good way to turn a working present into an upside-down one -- so
		// leave those exactly alone rather than guess.
		if (srcW <= 0 || srcH <= 0 || w <= 0 || h <= 0)
			return;

		// Fit srcW:srcH inside w:h without ever growing past it. Integer math in
		// 64-bit: 2048*600 already overflows nothing, but 1068*1152 * a future
		// 4K height would, and a silent wrap here is a garbage rect.
		int nw = w, nh = h;
		if ((long long)srcW * h > (long long)srcH * w)
			nh = (int)((long long)w * srcH / srcW);      // width-limited: bars top/bottom
		else
			nw = (int)((long long)h * srcW / srcH);      // height-limited: bars left/right
		// Publish the client size even when no correction is needed, so the mouse
		// mapping can tell "no letterbox" from "not measured yet".
		g_lbClientW.store(w, std::memory_order_relaxed);
		g_lbClientH.store(h, std::memory_order_relaxed);
		if (nw <= 0 || nh <= 0 || (nw == w && nh == h))
		{
			// Exact fit -- in-world 1068x600 into 16:9 lands here. Publish the
			// full rect so the mouse path maps 1:1 rather than through a stale
			// menu-sized rect.
			g_lbX.store(x, std::memory_order_relaxed);
			g_lbY.store(y, std::memory_order_relaxed);
			g_lbW.store(w, std::memory_order_relaxed);
			g_lbH.store(h, std::memory_order_relaxed);
			return;
		}

		const int nx = x + (w - nw) / 2;
		const int ny = y + (h - nh) / 2;

		// Mirror the image's own outer edge into each bar. The slice taken is the
		// bar's width converted back into SOURCE pixels, so the mirror runs at
		// the same scale as the image and the seam at the join lines up.
		const int oldMode = SetStretchBltMode(dc, HALFTONE);
		SetBrushOrgEx(dc, 0, 0, nullptr);
		using StretchRealFn = BOOL(WINAPI*)(HDC, int, int, int, int, HDC, int, int, int, int, DWORD);
		StretchRealFn sb = (StretchRealFn)g_probes[P_StretchBlt].real;
		if (sb && srcDc)
		{
			// Negative destination width is how GDI mirrors: the run starts at
			// the inner edge and grows outward, so the pixels nearest the image
			// are the ones that continue it.
			if (nx > x)
			{
				const int bar = nx - x;
				int slice = (int)((long long)bar * srcW / nw);
				if (slice < 1) slice = 1;
				if (slice > srcW) slice = srcW;
				sb(dc, nx, ny, -bar, nh, srcDc, sx, sy, slice, srcH, SRCCOPY);
			}
			if (nx + nw < x + w)
			{
				const int bar = (x + w) - (nx + nw);
				int slice = (int)((long long)bar * srcW / nw);
				if (slice < 1) slice = 1;
				if (slice > srcW) slice = srcW;
				sb(dc, nx + nw, ny, bar, nh, srcDc, sx + srcW - 1, sy, -slice, srcH, SRCCOPY);
			}
			if (ny > y)
			{
				const int bar = ny - y;
				int slice = (int)((long long)bar * srcH / nh);
				if (slice < 1) slice = 1;
				if (slice > srcH) slice = srcH;
				sb(dc, nx, ny, nw, -bar, srcDc, sx, sy, srcW, slice, SRCCOPY);
			}
			if (ny + nh < y + h)
			{
				const int bar = (y + h) - (ny + nh);
				int slice = (int)((long long)bar * srcH / nh);
				if (slice < 1) slice = 1;
				if (slice > srcH) slice = srcH;
				sb(dc, nx, ny + nh, nw, bar, srcDc, sx, sy + srcH - 1, srcW, -slice, SRCCOPY);
			}
		}
		SetStretchBltMode(dc, oldMode);

		g_lbX.store(nx, std::memory_order_relaxed);
		g_lbY.store(ny, std::memory_order_relaxed);
		g_lbW.store(nw, std::memory_order_relaxed);
		g_lbH.store(nh, std::memory_order_relaxed);

		x = nx; y = ny; w = nw; h = nh;
	}

	inline void Note(int i, void* ret)
	{
		Probe& p = g_probes[i];
		p.calls.fetch_add(1, std::memory_order_relaxed);
		void* expected = nullptr;
		p.firstCaller.compare_exchange_strong(expected, ret, std::memory_order_relaxed);
		p.lastCaller.store(ret, std::memory_order_relaxed);
	}

	// --- GDI -----------------------------------------------------------------
	using BitBltFn = BOOL(WINAPI*)(HDC, int, int, int, int, HDC, int, int, DWORD);
	using StretchBltFn = BOOL(WINAPI*)(HDC, int, int, int, int, HDC, int, int, int, int, DWORD);
	using StretchDIBitsFn = int(WINAPI*)(HDC, int, int, int, int, int, int, int, int,
	                                     const void*, const BITMAPINFO*, UINT, DWORD);
	using SetDIBitsToDeviceFn = int(WINAPI*)(HDC, int, int, DWORD, DWORD, int, int, UINT, UINT,
	                                         const void*, const BITMAPINFO*, UINT);
	using PatBltFn = BOOL(WINAPI*)(HDC, int, int, int, int, DWORD);
	using SwapBuffersFn = BOOL(WINAPI*)(HDC);
	using SwapLayerFn = BOOL(WINAPI*)(HDC, UINT);
	using VoidFn = void(WINAPI*)(void);
	using DrawPixelsFn = void(WINAPI*)(int, int, unsigned, unsigned, const void*);
	using DwmFlushFn = HRESULT(WINAPI*)(void);
	using CreateDIBSectionFn = HBITMAP(WINAPI*)(HDC, const BITMAPINFO*, UINT, void**, HANDLE, DWORD);

	BOOL WINAPI H_BitBlt(HDC a, int b, int c, int d, int e, HDC f, int g, int h, DWORD i)
	{ Note(P_BitBlt, _ReturnAddress()); return ((BitBltFn)g_probes[P_BitBlt].real)(a,b,c,d,e,f,g,h,i); }

	BOOL WINAPI H_StretchBlt(HDC a, int b, int c, int d, int e, HDC f, int g, int h, int i, int j, DWORD k)
	{
		Note(P_StretchBlt, _ReturnAddress());
		// f is hdcSrc -- the memory DC holding the frame DIB. Capture reads it
		// BEFORE the blit; a no-op unless a capture was requested.
		// e/j are the signed DESTINATION and SOURCE heights: GDI flips the image
		// vertically when their signs differ, and D2 uses exactly that to present
		// a bottom-up backbuffer. Reading raw DIB bits bypasses the blit, so the
		// capture has to reapply the flip itself or the frame comes out mirrored.
		D2Capture_OnStretchBlt((void*)f, e, j);
		// Correct the aspect BEFORE recording, so the reported destination is
		// the rectangle the frame actually lands in -- a report of the rect we
		// were asked for would be a report of something that did not happen.
		if (D2GameWindow_IsFullscreen())
			LetterboxDest(a, b, c, d, e, f, g, h, i, j);
		// Raw and unsigned-corrected nowhere: the heights keep their sign so a
		// flipped present stays visible as one in the report.
		g_blitDstW.store(d, std::memory_order_relaxed);
		g_blitDstH.store(e, std::memory_order_relaxed);
		g_blitSrcW.store(i, std::memory_order_relaxed);
		g_blitSrcH.store(j, std::memory_order_relaxed);
		return ((StretchBltFn)g_probes[P_StretchBlt].real)(a,b,c,d,e,f,g,h,i,j,k);
	}

	int WINAPI H_StretchDIBits(HDC a, int b, int c, int d, int e, int f, int g, int h, int i,
	                           const void* j, const BITMAPINFO* k, UINT l, DWORD m)
	{ Note(P_StretchDIBits, _ReturnAddress()); return ((StretchDIBitsFn)g_probes[P_StretchDIBits].real)(a,b,c,d,e,f,g,h,i,j,k,l,m); }

	int WINAPI H_SetDIBitsToDevice(HDC a, int b, int c, DWORD d, DWORD e, int f, int g, UINT h, UINT i,
	                               const void* j, const BITMAPINFO* k, UINT l)
	{ Note(P_SetDIBitsToDevice, _ReturnAddress()); return ((SetDIBitsToDeviceFn)g_probes[P_SetDIBitsToDevice].real)(a,b,c,d,e,f,g,h,i,j,k,l); }

	BOOL WINAPI H_PatBlt(HDC a, int b, int c, int d, int e, DWORD f)
	{ Note(P_PatBlt, _ReturnAddress()); return ((PatBltFn)g_probes[P_PatBlt].real)(a,b,c,d,e,f); }

	BOOL WINAPI H_SwapBuffersGdi(HDC a)
	{ Note(P_SwapBuffersGdi, _ReturnAddress()); return ((SwapBuffersFn)g_probes[P_SwapBuffersGdi].real)(a); }

	// --- OpenGL --------------------------------------------------------------
	BOOL WINAPI H_wglSwapBuffers(HDC a)
	{ Note(P_wglSwapBuffers, _ReturnAddress()); return ((SwapBuffersFn)g_probes[P_wglSwapBuffers].real)(a); }

	BOOL WINAPI H_wglSwapLayerBuffers(HDC a, UINT b)
	{ Note(P_wglSwapLayerBuffers, _ReturnAddress()); return ((SwapLayerFn)g_probes[P_wglSwapLayerBuffers].real)(a,b); }

	void WINAPI H_glFlush(void)
	{ Note(P_glFlush, _ReturnAddress()); ((VoidFn)g_probes[P_glFlush].real)(); }

	void WINAPI H_glFinish(void)
	{ Note(P_glFinish, _ReturnAddress()); ((VoidFn)g_probes[P_glFinish].real)(); }

	void WINAPI H_glDrawPixels(int a, int b, unsigned c, unsigned d, const void* e)
	{ Note(P_glDrawPixels, _ReturnAddress()); ((DrawPixelsFn)g_probes[P_glDrawPixels].real)(a,b,c,d,e); }

	// Records every DIB the game allocates, with its DIRECT pixel pointer --
	// which is what makes reading frames by reference possible at all.
	HBITMAP WINAPI H_CreateDIBSection(HDC a, const BITMAPINFO* bi, UINT usage,
	                                  void** bits, HANDLE sec, DWORD off)
	{
		Note(P_CreateDIBSection, _ReturnAddress());
		HBITMAP r = ((CreateDIBSectionFn)g_probes[P_CreateDIBSection].real)(a, bi, usage, bits, sec, off);
		if (r && bits && *bits && bi)
			D2Capture_NoteDib((void*)r, *bits, bi->bmiHeader.biWidth,
			                  bi->bmiHeader.biHeight, bi->bmiHeader.biBitCount);
		return r;
	}

	// --- compositor ----------------------------------------------------------
	HRESULT WINAPI H_DwmFlush(void)
	{ Note(P_DwmFlush, _ReturnAddress()); return ((DwmFlushFn)g_probes[P_DwmFlush].real)(); }

	void ProbeLog(const char* m)
	{
		FILE* f = nullptr;
		if (fopen_s(&f, "C:\\Users\\benam\\source\\cpp\\D2MOO\\conformance\\behavioral\\overlay_gl_log.txt", "a") == 0 && f)
		{
			fprintf(f, "[probe] %s\n", m);
			fclose(f);
		}
	}

	void Attach(int idx, const wchar_t* mod, const char* fn, void* hook)
	{
		HMODULE h = GetModuleHandleW(mod);
		if (!h)
			return;
		void* real = (void*)GetProcAddress(h, fn);
		if (!real)
			return;
		g_probes[idx].real = real;
		DetourAttach(&g_probes[idx].real, hook);
	}

	std::atomic<bool> g_installed{ false };

	DWORD WINAPI ProbeInstallThread(LPVOID)
	{
		// PHASE 1 -- GDI, IMMEDIATELY. gdi32.dll is loaded in every GUI process
		// long before any of our code runs, so there is nothing to wait for.
		//
		// These used to sit BEHIND the opengl32 wait below, which cost up to 30
		// seconds of blindness. It went unnoticed only because the real
		// glide3x.dll statically imports OPENGL32, so the wait was satisfied at
		// process start and returned on the first check. Replace glide3x with a
		// stub that does not import GL -- as the container build does -- and
		// nothing loads opengl32 at all: the thread burned the entire 600x50ms
		// timeout, CreateDIBSection was never seen (so the orientation registry
		// stayed empty), and the game rendered its whole menu unhooked. Frames
		// only appeared once the timeout expired, which looked like "the menu
		// does not stream". A capture hook must never depend on an unrelated
		// renderer's library being present.
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		Attach(P_BitBlt,             L"gdi32.dll",    "BitBlt",             (void*)H_BitBlt);
		Attach(P_StretchBlt,         L"gdi32.dll",    "StretchBlt",         (void*)H_StretchBlt);
		Attach(P_StretchDIBits,      L"gdi32.dll",    "StretchDIBits",      (void*)H_StretchDIBits);
		Attach(P_SetDIBitsToDevice,  L"gdi32.dll",    "SetDIBitsToDevice",  (void*)H_SetDIBitsToDevice);
		Attach(P_PatBlt,             L"gdi32.dll",    "PatBlt",             (void*)H_PatBlt);
		Attach(P_SwapBuffersGdi,     L"gdi32.dll",    "SwapBuffers",        (void*)H_SwapBuffersGdi);
		Attach(P_CreateDIBSection,   L"gdi32.dll",    "CreateDIBSection",   (void*)H_CreateDIBSection);
		const LONG errGdi = DetourTransactionCommit();

		char b[256];
		_snprintf_s(b, sizeof(b), _TRUNCATE, "gdi install commit=%ld", errGdi);
		ProbeLog(b);

		// PHASE 2 -- GL and DWM, which genuinely may not be loaded yet. Bounded
		// so a configuration that never loads GL (every GDI-only build) does not
		// spin forever; nothing above depends on the outcome.
		for (int i = 0; i < 600 && !GetModuleHandleW(L"opengl32.dll"); ++i)
			Sleep(50);
		// dwmapi is not loaded by default -- load it so the hook can attach.
		LoadLibraryW(L"dwmapi.dll");

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		Attach(P_wglSwapBuffers,     L"opengl32.dll", "wglSwapBuffers",     (void*)H_wglSwapBuffers);
		Attach(P_wglSwapLayerBuffers,L"opengl32.dll", "wglSwapLayerBuffers",(void*)H_wglSwapLayerBuffers);
		Attach(P_glFlush,            L"opengl32.dll", "glFlush",            (void*)H_glFlush);
		Attach(P_glFinish,           L"opengl32.dll", "glFinish",           (void*)H_glFinish);
		Attach(P_glDrawPixels,       L"opengl32.dll", "glDrawPixels",       (void*)H_glDrawPixels);
		Attach(P_DwmFlush,           L"dwmapi.dll",   "DwmFlush",           (void*)H_DwmFlush);
		const LONG errGl = DetourTransactionCommit();

		_snprintf_s(b, sizeof(b), _TRUNCATE, "gl/dwm install commit=%ld", errGl);
		ProbeLog(b);
		return 0;
	}

	const char* Names[P_COUNT] = {
		"BitBlt", "StretchBlt", "StretchDIBits", "SetDIBitsToDevice", "PatBlt",
		"gdi32!SwapBuffers",
		"wglSwapBuffers", "wglSwapLayerBuffers", "glFlush", "glFinish", "glDrawPixels",
		"DwmFlush", "CreateDIBSection",
	};

	// Resolved HERE, on the query thread -- never in a hook. GetModuleHandleEx
	// takes the loader lock; calling it thousands of times a second inside a
	// present path would be both slow and a deadlock hazard.
	void ModuleOf(void* addr, char* out, int cch)
	{
		out[0] = 0;
		if (!addr)
			return;
		HMODULE h = nullptr;
		if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
		                        | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		                        (LPCWSTR)addr, &h) || !h)
			return;
		wchar_t path[MAX_PATH] = L"";
		if (!GetModuleFileNameW(h, path, MAX_PATH))
			return;
		const wchar_t* base = wcsrchr(path, L'\\');
		base = base ? base + 1 : path;
		WideCharToMultiByte(CP_UTF8, 0, base, -1, out, cch, nullptr, nullptr);
	}
}

// Where the image actually lands inside the client, for the mouse mapping in
// D2Debugger.gamewindow.cpp. Returns 0 until a full-screen blit has been seen,
// so the caller can leave input alone rather than map through zeros.
extern "C" int D2Probe_LetterboxRect(int* x, int* y, int* w, int* h,
                                     int* clientW, int* clientH)
{
	const int cw = g_lbClientW.load(std::memory_order_relaxed);
	const int lw = g_lbW.load(std::memory_order_relaxed);
	if (cw <= 0 || lw <= 0)
		return 0;
	if (x) *x = g_lbX.load(std::memory_order_relaxed);
	if (y) *y = g_lbY.load(std::memory_order_relaxed);
	if (w) *w = lw;
	if (h) *h = g_lbH.load(std::memory_order_relaxed);
	if (clientW) *clientW = cw;
	if (clientH) *clientH = g_lbClientH.load(std::memory_order_relaxed);
	return 1;
}

extern "C" void D2Probe_StartInstall()
{
	if (g_installed.exchange(true))
		return;
	if (HANDLE h = CreateThread(nullptr, 0, ProbeInstallThread, nullptr, 0, nullptr))
		CloseHandle(h);
}

// JSON for GET /capture/probe. Includes a per-second RATE derived from the
// caller's own sampling interval, because "called at all" and "called 60x a
// second" mean completely different things: one is init, the other is the
// present path.
extern "C" int D2Probe_Report(char* buf, int cch)
{
	static unsigned long long s_prev[P_COUNT] = { 0 };
	static DWORD s_prevTick = 0;

	const DWORD now = GetTickCount();
	const double dt = s_prevTick ? (now - s_prevTick) / 1000.0 : 0.0;

	int n = _snprintf_s(buf, cch, _TRUNCATE, "{\"ok\":true,\"sampleSec\":%.2f,\"apis\":[", dt);
	for (int i = 0; i < P_COUNT; ++i)
	{
		const unsigned long long c = g_probes[i].calls.load(std::memory_order_relaxed);
		const double rate = (dt > 0.05) ? (double)(c - s_prev[i]) / dt : 0.0;
		s_prev[i] = c;

		char firstMod[64] = "", lastMod[64] = "";
		ModuleOf(g_probes[i].firstCaller.load(std::memory_order_relaxed), firstMod, sizeof(firstMod));
		ModuleOf(g_probes[i].lastCaller.load(std::memory_order_relaxed), lastMod, sizeof(lastMod));

		n += _snprintf_s(buf + n, cch - n, _TRUNCATE,
			"%s{\"api\":\"%s\",\"hooked\":%s,\"calls\":%llu,\"perSec\":%.1f,"
			"\"firstCaller\":\"%s\",\"lastCaller\":\"%s\"}",
			i ? "," : "", Names[i], g_probes[i].real ? "true" : "false",
			c, rate, firstMod, lastMod);
		if (n >= cch - 64)
			break;
	}
	s_prevTick = now;
	n += _snprintf_s(buf + n, cch - n, _TRUNCATE, "],");

	// See g_blitDstW. Reported next to the rates because the two answer the same
	// kind of question: the rate says the game is presenting, this says WHERE to
	// and at what size.
	n += _snprintf_s(buf + n, cch - n, _TRUNCATE,
		"\"blit\":{\"dstX\":%d,\"dstY\":%d,\"dstW\":%d,\"dstH\":%d,"
		"\"srcW\":%d,\"srcH\":%d,\"clientW\":%d,\"clientH\":%d},",
		g_lbX.load(std::memory_order_relaxed),
		g_lbY.load(std::memory_order_relaxed),
		g_blitDstW.load(std::memory_order_relaxed),
		g_blitDstH.load(std::memory_order_relaxed),
		g_blitSrcW.load(std::memory_order_relaxed),
		g_blitSrcH.load(std::memory_order_relaxed),
		g_lbClientW.load(std::memory_order_relaxed),
		g_lbClientH.load(std::memory_order_relaxed));

	n += (D2Capture_DibReport(buf + n, cch - n), (int)strlen(buf + n));
	_snprintf_s(buf + n, cch - n, _TRUNCATE, "}");
	return 1;
}
