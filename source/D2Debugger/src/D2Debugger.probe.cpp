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

	n += (D2Capture_DibReport(buf + n, cch - n), (int)strlen(buf + n));
	_snprintf_s(buf + n, cch - n, _TRUNCATE, "}");
	return 1;
}
