// D2Debugger.hostwindow.cpp -- the debugger's OWN top-level window: which
// monitor it lives on, and filling that monitor exactly.
//
// (D2Debugger.gamewindow.cpp is the mirror of this file for the GAME's window.
// Same idea, opposite subject: that one gets a window OUT of the way, this one
// puts ours exactly where it belongs.)
//
// WHAT IT REPLACES. The host window was created WS_OVERLAPPEDWINDOW at a
// hardcoded 30,30,900,720 -- a size chosen when the debugger was a small side
// panel. Now the game itself renders inside it (D2Debugger.gamepanel.cpp), so
// the host is the whole working surface and wants the whole monitor.
//
// THE TRAP: A 2560x1440 MONITOR DOES NOT REPORT 2560x1440.
//
// Measured on this box: the 1440p display is the PRIMARY and is scaled to 125%,
// so a DPI-UNAWARE process is told its rect is 2048x1152 -- exactly 2560/1.25 x
// 1440/1.25. Two consequences, both of which look like bugs elsewhere:
//
//   1. Picking "the 2560x1440 monitor" by comparing GetMonitorInfo's rect
//      NEVER MATCHES. So the monitor is identified by its PHYSICAL mode
//      (EnumDisplaySettingsW, which reports the real 2560x1440 whatever the
//      caller's DPI awareness), not by the virtualized rect.
//   2. Filling the virtualized 2048x1152 rect does fill the screen, but DWM
//      then stretches the result by 1.25 -- soft text, and the game panel's 1:1
//      lock becomes a lie, since its pixel-exact frame is resampled on the way
//      to the glass.
//
// So the render thread declares itself DPI-aware and we get a true 2560x1440
// client area. THREAD-scoped, never process-scoped -- that distinction is the
// whole safety argument:
//
//   * ImGui_ImplWin32_EnableDpiAwareness() is right there and is NOT used: it
//     falls back to SetProcessDpiAwareness on 8.1, and process-wide awareness
//     inside GAME.EXE would re-base the coordinate space of the game's own
//     window, cnc-ddraw's geometry, and everything fun-doc's game_window.py
//     computes for it from outside (that module is unaware, like every other
//     ordinary process). We only ever want OUR window in real pixels.
//   * Every touch of the GAME's window therefore has to happen UNAWARE, or a
//     rect saved from one thread and restored from another differs by 1.25x.
//     D2Host_ScopedUnaware exists for exactly those call sites.
//
// The awareness switch is defeasible from the ini (DpiAware=0) without a
// rebuild, because "everything is 20% off" is a horrible thing to be stuck with
// on a machine we cannot test here.
//
// PERSISTED in imgui.ini under [D2Host][Settings]:
//   Fill=1        borderless, filling a whole monitor (F11 toggles)
//   DpiAware=1    render thread declares per-monitor DPI awareness

#include "imgui.h"
#include "imgui_internal.h"     // ImHashStr, AddSettingsHandler, MarkIniSettingsDirty
#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>

// Win32 DPI plumbing, declared the way imgui_impl_win32.cpp does so this builds
// against an SDK that predates the per-monitor-v2 constant.
#ifndef _DPI_AWARENESS_CONTEXTS_
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_UNAWARE              (DPI_AWARENESS_CONTEXT)-1
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    (DPI_AWARENESS_CONTEXT)-3
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 (DPI_AWARENESS_CONTEXT)-4
#endif
typedef DPI_AWARENESS_CONTEXT(WINAPI* PFN_SetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
typedef UINT(WINAPI* PFN_GetDpiForWindow)(HWND);

namespace
{
	// The display we want when nothing else decides it. Not a hard requirement:
	// an exact match wins, otherwise the largest panel does, so this works on a
	// box that has no 1440p screen at all.
	constexpr int kPreferredW = 2560;
	constexpr int kPreferredH = 1440;

	bool g_fill = true;             // persisted
	bool g_wantDpiAware = true;     // persisted
	bool g_dpiAwareApplied = false;
	bool g_saved = false;
	LONG g_savedStyle = 0;
	LONG g_savedExStyle = 0;
	RECT g_savedRect{};
	float g_dpiScale = 1.0f;
	// Drift watchdog state (see D2Host_Tick, below). Declared here with the rest
	// of the file's state so D2Host_ApplyFill can reset it.
	constexpr int kMaxCorrections = 8;
	int  g_corrections = 0;
	bool g_correctionGiveUp = false;
	int  g_lastSizeW = 0, g_lastSizeH = 0;

	void HostLog(const char* fmt, ...)
	{
		char buf[512];
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);
		char line[600];
		snprintf(line, sizeof(line), "[hostwindow] %s\n", buf);
		OutputDebugStringA(line);
		FILE* f = nullptr;
		if (fopen_s(&f, "C:\\Users\\benam\\source\\cpp\\D2MOO\\conformance\\behavioral\\overlay_gl_log.txt", "a") == 0 && f)
		{
			fputs(line, f);
			fclose(f);
		}
	}

	PFN_SetThreadDpiAwarenessContext SetThreadDpiAwarenessContextFn()
	{
		static PFN_SetThreadDpiAwarenessContext fn = []() -> PFN_SetThreadDpiAwarenessContext {
			HMODULE u32 = ::GetModuleHandleA("user32.dll");
			if (!u32)
				u32 = ::LoadLibraryA("user32.dll");
			return u32 ? (PFN_SetThreadDpiAwarenessContext)::GetProcAddress(
				u32, "SetThreadDpiAwarenessContext") : nullptr;
		}();
		return fn;
	}

	struct MonitorEntry
	{
		HMONITOR handle = nullptr;
		RECT     rect{};            // as this THREAD sees it (virtualized if unaware)
		RECT     work{};
		int      physW = 0;         // real pixels, whatever our awareness is
		int      physH = 0;
		bool     primary = false;
		wchar_t  device[CCHDEVICENAME]{};
	};

	BOOL CALLBACK CollectMonitor(HMONITOR h, HDC, LPRECT, LPARAM user)
	{
		auto* out = (std::vector<MonitorEntry>*)user;
		MONITORINFOEXW mi{};
		mi.cbSize = sizeof(mi);
		if (!GetMonitorInfoW(h, &mi))
			return TRUE;

		MonitorEntry e;
		e.handle = h;
		e.rect = mi.rcMonitor;
		e.work = mi.rcWork;
		e.primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
		wcsncpy_s(e.device, mi.szDevice, _TRUNCATE);

		// THE PHYSICAL MODE, which is the only size that is the same number for
		// an aware and an unaware caller. The rect above is not: on a 125%
		// display an unaware thread is told 2048x1152 for a 2560x1440 panel.
		DEVMODEW dm{};
		dm.dmSize = sizeof(dm);
		if (EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm))
		{
			e.physW = (int)dm.dmPelsWidth;
			e.physH = (int)dm.dmPelsHeight;
		}
		else
		{
			e.physW = e.rect.right - e.rect.left;
			e.physH = e.rect.bottom - e.rect.top;
		}
		out->push_back(e);
		return TRUE;
	}

	std::vector<MonitorEntry> Monitors()
	{
		std::vector<MonitorEntry> v;
		EnumDisplayMonitors(nullptr, nullptr, CollectMonitor, (LPARAM)&v);
		return v;
	}

	// Exact preferred mode wins; otherwise the largest panel by physical area;
	// otherwise (no monitors enumerated at all) nothing, and the caller leaves
	// the window alone rather than guessing.
	bool PickMonitor(MonitorEntry& out)
	{
		std::vector<MonitorEntry> mons = Monitors();
		if (mons.empty())
			return false;

		const MonitorEntry* best = nullptr;
		for (const MonitorEntry& m : mons)
			if (m.physW == kPreferredW && m.physH == kPreferredH)
			{
				best = &m;
				break;
			}
		if (!best)
			for (const MonitorEntry& m : mons)
				if (!best || (long long)m.physW * m.physH >
				             (long long)best->physW * best->physH)
					best = &m;

		out = *best;
		char dev[CCHDEVICENAME * 2]{};
		WideCharToMultiByte(CP_UTF8, 0, out.device, -1, dev, sizeof(dev) - 1, nullptr, nullptr);
		HostLog("monitor %s physical %dx%d, this thread sees %dx%d at %ld,%ld%s",
		        dev, out.physW, out.physH,
		        (int)(out.rect.right - out.rect.left), (int)(out.rect.bottom - out.rect.top),
		        out.rect.left, out.rect.top, out.primary ? " (primary)" : "");
		return true;
	}

	void SaveOnce(HWND h)
	{
		if (g_saved)
			return;
		g_savedStyle = GetWindowLongW(h, GWL_STYLE);
		g_savedExStyle = GetWindowLongW(h, GWL_EXSTYLE);
		GetWindowRect(h, &g_savedRect);
		g_saved = true;
	}

	// ---- persisted settings ------------------------------------------------
	void SettingsClearAll(ImGuiContext*, ImGuiSettingsHandler*) {}

	void* SettingsReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char*)
	{
		return (void*)(intptr_t)1;
	}

	void SettingsReadLine(ImGuiContext*, ImGuiSettingsHandler*, void*, const char* line)
	{
		int v = 0;
		if      (sscanf_s(line, "Fill=%d", &v) == 1)     g_fill = v != 0;
		else if (sscanf_s(line, "DpiAware=%d", &v) == 1) g_wantDpiAware = v != 0;
	}

	void SettingsWriteAll(ImGuiContext*, ImGuiSettingsHandler* h, ImGuiTextBuffer* buf)
	{
		buf->appendf("[%s][Settings]\n", h->TypeName);
		buf->appendf("Fill=%d\n", g_fill ? 1 : 0);
		buf->appendf("DpiAware=%d\n", g_wantDpiAware ? 1 : 0);
		buf->append("\n");
	}
}

// Read the ini ourselves, BEFORE ImGui exists.
//
// The DPI decision has to be made before the window is created, and the window
// is created before ImGui::CreateContext -- so the settings handler below,
// which is the normal way to read this, has not run and cannot. One tiny
// hand-parse of the block we own is the price; the handler is still what WRITES
// it, so there is exactly one format.
extern "C" void D2Host_PreloadSettings(const char* iniPath)
{
	FILE* f = nullptr;
	if (fopen_s(&f, iniPath ? iniPath : "imgui.ini", "r") != 0 || !f)
		return;
	char line[256];
	bool inBlock = false;
	while (fgets(line, sizeof(line), f))
	{
		if (line[0] == '[')
			inBlock = (strncmp(line, "[D2Host][", 9) == 0);
		else if (inBlock)
		{
			int v = 0;
			if      (sscanf_s(line, "Fill=%d", &v) == 1)     g_fill = v != 0;
			else if (sscanf_s(line, "DpiAware=%d", &v) == 1) g_wantDpiAware = v != 0;
		}
	}
	fclose(f);
	HostLog("preloaded settings: Fill=%d DpiAware=%d", g_fill ? 1 : 0, g_wantDpiAware ? 1 : 0);
}

// Declare THIS THREAD per-monitor DPI aware. Must run before the host window is
// created, and deliberately never touches the process-wide setting -- see the
// file header for why that would be a much bigger and much worse change.
extern "C" void D2Host_ThreadDpiAware()
{
	if (!g_wantDpiAware || g_dpiAwareApplied)
		return;
	if (PFN_SetThreadDpiAwarenessContext fn = SetThreadDpiAwarenessContextFn())
	{
		if (fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) != nullptr)
		{
			g_dpiAwareApplied = true;
			HostLog("render thread is per-monitor DPI aware (v2)");
			return;
		}
	}
	// Windows 10 1607 is where SetThreadDpiAwarenessContext appears. Older than
	// that, stay unaware and fill the virtualized rect -- still fills the
	// screen, just resampled. NOT worth reaching for the process-wide API.
	HostLog("SetThreadDpiAwarenessContext unavailable -- staying DPI unaware");
}

// Run a block of code as if we were an ordinary unaware process, and put the
// old context back. Required around every call that reads or writes the GAME's
// window geometry: that window belongs to unaware threads, and fun-doc's
// game_window.py drives it from an unaware process, so its rects only mean one
// thing in unaware coordinates. A rect saved aware and restored unaware (or the
// reverse) is off by exactly the scale factor -- 1.25 here -- which reads as a
// window that "drifts" rather than as a units bug.
//
// Push/pop rather than a C++ RAII type on purpose: the call sites are in other
// translation units, and a header for two calls is not worth it. Returns an
// opaque token; pass it straight back. A null token is valid and means "we were
// never aware, nothing to undo".
extern "C" void* D2Host_PushUnaware()
{
	if (!g_dpiAwareApplied)
		return nullptr;
	PFN_SetThreadDpiAwarenessContext fn = SetThreadDpiAwarenessContextFn();
	return fn ? (void*)fn(DPI_AWARENESS_CONTEXT_UNAWARE) : nullptr;
}

extern "C" void D2Host_PopUnaware(void* token)
{
	if (!token)
		return;
	if (PFN_SetThreadDpiAwarenessContext fn = SetThreadDpiAwarenessContextFn())
		fn((DPI_AWARENESS_CONTEXT)token);
}

extern "C" int D2Host_IsDpiAware() { return g_dpiAwareApplied ? 1 : 0; }
extern "C" float D2Host_DpiScale() { return g_dpiScale; }

// The scale ImGui should render at so the UI keeps its apparent size. Unaware,
// this is 1.0 and DWM does the scaling; aware, it is the monitor's real factor
// and we do it ourselves -- crisply, and without touching the game panel's
// image, which is drawn at texture pixels either way.
extern "C" float D2Host_RefreshDpiScale(HWND hwnd)
{
	float s = 1.0f;
	if (g_dpiAwareApplied && hwnd)
	{
		static PFN_GetDpiForWindow getDpi = []() -> PFN_GetDpiForWindow {
			HMODULE u32 = ::GetModuleHandleA("user32.dll");
			return u32 ? (PFN_GetDpiForWindow)::GetProcAddress(u32, "GetDpiForWindow") : nullptr;
		}();
		if (getDpi)
		{
			const UINT dpi = getDpi(hwnd);
			if (dpi >= 48)                     // sanity: 0 means the call failed
				s = (float)dpi / 96.0f;
		}
	}
	g_dpiScale = s;
	return s;
}

extern "C" int D2Host_IsFilled() { return g_fill ? 1 : 0; }

// Borderless, covering one whole monitor -- taskbar included, which is what
// "perfectly fills the screen" has to mean for a top-most working surface.
//
// `preferCurrent` fills the monitor the window is already on (the F11 path, so
// a toggle never yanks the window to another screen); at startup it is false
// and the preferred/largest monitor is chosen.
extern "C" void D2Host_ApplyFill(HWND hwnd, int preferCurrent)
{
	if (!hwnd)
		return;
	SaveOnce(hwnd);

	MonitorEntry m;
	bool have = false;
	if (preferCurrent)
	{
		HMONITOR h = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		for (const MonitorEntry& e : Monitors())
			if (e.handle == h) { m = e; have = true; break; }
	}
	if (!have && !PickMonitor(m))
		return;

	const LONG kill = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX |
	                  WS_MAXIMIZEBOX | WS_SYSMENU | WS_BORDER | WS_DLGFRAME;
	const LONG killEx = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE |
	                    WS_EX_CLIENTEDGE | WS_EX_STATICEDGE;
	SetWindowLongW(hwnd, GWL_STYLE, (g_savedStyle & ~kill) | WS_POPUP | WS_VISIBLE);
	SetWindowLongW(hwnd, GWL_EXSTYLE, g_savedExStyle & ~killEx);

	const int w = m.rect.right - m.rect.left;
	const int h = m.rect.bottom - m.rect.top;
	SetWindowPos(hwnd, HWND_TOPMOST, m.rect.left, m.rect.top, w, h,
	             SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);
	g_fill = true;
	// An explicit fill is a fresh start for the drift watchdog: whatever
	// exhausted its budget last time, the operator has just asked for this
	// again and deserves the full budget of corrections.
	g_corrections = 0;
	g_correctionGiveUp = false;
	D2Host_RefreshDpiScale(hwnd);

	// READ IT BACK rather than trusting the calls. Borderless is asserted by
	// three separate APIs here and a style that silently did not take is
	// indistinguishable from one that did until you look at the screen -- the
	// same failure mode that forced fun-doc's _set_borderless to verify.
	RECT cr{};
	GetClientRect(hwnd, &cr);
	const LONG got = GetWindowLongW(hwnd, GWL_STYLE);
	HostLog("fill -> client %dx%d (want %dx%d), caption=%s, dpi scale %.2f",
	        (int)cr.right, (int)cr.bottom, w, h,
	        (got & WS_CAPTION) ? "STILL PRESENT" : "gone", g_dpiScale);
	if (cr.right != w || cr.bottom != h)
		HostLog("WARNING: client area is not the monitor size -- "
		        "expect letterboxing or a scaled surface");
}

// Back to an ordinary framed window at whatever geometry it had before the
// first fill. Not merely cosmetic: with no title bar and no sysmenu there is
// nothing to grab, so a window stuck borderless off-screen would be lost.
extern "C" void D2Host_RestoreWindowed(HWND hwnd)
{
	if (!hwnd || !g_saved)
		return;
	SetWindowLongW(hwnd, GWL_STYLE, g_savedStyle);
	SetWindowLongW(hwnd, GWL_EXSTYLE, g_savedExStyle);
	SetWindowPos(hwnd, HWND_TOPMOST,
	             g_savedRect.left, g_savedRect.top,
	             g_savedRect.right - g_savedRect.left,
	             g_savedRect.bottom - g_savedRect.top,
	             SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);
	g_fill = false;
	D2Host_RefreshDpiScale(hwnd);
	HostLog("restored windowed %ldx%ld at %ld,%ld",
	        g_savedRect.right - g_savedRect.left,
	        g_savedRect.bottom - g_savedRect.top,
	        g_savedRect.left, g_savedRect.top);
}

// ---- self-healing ----------------------------------------------------------
//
// WHY THIS EXISTS. The fill provably lands (the read-back above logs client
// 2560x1440) and the window was later found at 1112x1380 -- borderless, so with
// no title bar to drag and no grips to resize, i.e. stuck. 1112 is not a mystery
// number: it is exactly the game panel's "grow the host" arithmetic for a
// 1068-wide frame, so SOMETHING shrinks the host and that block then "repairs"
// it to panel-width, which is worse than leaving it alone.
//
// Rather than only fixing the one resizer we can name, the fill asserts itself:
// while Fill is on, a client area that is not the monitor's is put back. That
// covers whatever else grows into this file later, which matters because a
// borderless window that ends up the wrong size is not merely cosmetic -- it is
// unrecoverable from the mouse alone.
//
// BOUNDED, because two things that each insist on a different size would
// otherwise thrash forever at 60fps. After kMaxCorrections the loop gives up
// LOUDLY and leaves the window alone; the log then names the size that keeps
// coming back, which is the thing you actually need to know.

// Called from WM_SIZE. Every resize of the host is worth a line: this window is
// resized by code the operator cannot see, and the whole difficulty above was
// not knowing WHICH resize was the bad one.
extern "C" void D2Host_NoteSize(int w, int h)
{
	if (w == g_lastSizeW && h == g_lastSizeH)
		return;
	g_lastSizeW = w;
	g_lastSizeH = h;
	HostLog("WM_SIZE -> client %dx%d%s", w, h, g_fill ? " (fill mode ON)" : "");
}

// Called once per frame. Cheap: a GetClientRect and two compares.
extern "C" void D2Host_Tick(HWND hwnd)
{
	if (!hwnd || !g_fill || g_correctionGiveUp)
		return;
	RECT cr{};
	if (!GetClientRect(hwnd, &cr))
		return;

	HMONITOR mh = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFOEXW mi{};
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfoW(mh, &mi))
		return;
	const int wantW = mi.rcMonitor.right - mi.rcMonitor.left;
	const int wantH = mi.rcMonitor.bottom - mi.rcMonitor.top;
	if (cr.right == wantW && cr.bottom == wantH)
		return;                                  // the normal case, every frame

	if (++g_corrections > kMaxCorrections)
	{
		g_correctionGiveUp = true;
		HostLog("GIVING UP re-asserting the fill: client keeps returning to %dx%d "
		        "(want %dx%d) after %d corrections. Something else owns this "
		        "window's size. F11 restores a normal framed window.",
		        (int)cr.right, (int)cr.bottom, wantW, wantH, kMaxCorrections);
		return;
	}
	HostLog("fill drifted to %dx%d (want %dx%d) -- re-asserting (%d/%d)",
	        (int)cr.right, (int)cr.bottom, wantW, wantH, g_corrections, kMaxCorrections);
	SetWindowPos(hwnd, HWND_TOPMOST, mi.rcMonitor.left, mi.rcMonitor.top,
	             wantW, wantH, SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

// F11.
extern "C" void D2Host_ToggleFill(HWND hwnd)
{
	if (g_fill)
		D2Host_RestoreWindowed(hwnd);
	else
		D2Host_ApplyFill(hwnd, /*preferCurrent=*/1);
	if (ImGui::GetCurrentContext())
		ImGui::MarkIniSettingsDirty();
}

// Called at startup, after the window exists. Honours the persisted choice.
extern "C" void D2Host_ApplyStartupLayout(HWND hwnd)
{
	if (g_fill)
	{
		D2Host_ApplyFill(hwnd, /*preferCurrent=*/0);
	}
	else
	{
		// The old hardcoded placement, kept as the windowed default so
		// unticking Fill lands somewhere sane rather than at 0,0.
		SetWindowPos(hwnd, HWND_TOPMOST, 30, 30, 900, 720, SWP_SHOWWINDOW);
		D2Host_RefreshDpiScale(hwnd);
	}
}

// WM_DPICHANGED: the window moved to a monitor with a different scale (only
// reachable while windowed, but reachable). Windows hands us the rect it wants;
// take it, then let the caller re-scale the ImGui style.
extern "C" void D2Host_OnDpiChanged(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	const RECT* suggested = (const RECT*)lParam;
	if (suggested)
		SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
		             suggested->right - suggested->left,
		             suggested->bottom - suggested->top,
		             SWP_NOZORDER | SWP_NOACTIVATE);
	const UINT dpi = LOWORD(wParam);
	g_dpiScale = dpi >= 48 ? (float)dpi / 96.0f : 1.0f;
	HostLog("WM_DPICHANGED -> scale %.2f", g_dpiScale);
}

void D2Host_RegisterSettings()
{
	ImGuiSettingsHandler h;
	h.TypeName = "D2Host";
	h.TypeHash = ImHashStr("D2Host");
	h.ClearAllFn = SettingsClearAll;
	h.ReadOpenFn = SettingsReadOpen;
	h.ReadLineFn = SettingsReadLine;
	h.WriteAllFn = SettingsWriteAll;
	ImGui::AddSettingsHandler(&h);
}
