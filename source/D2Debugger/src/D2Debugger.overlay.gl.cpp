// D2Debugger.overlay.gl.cpp -- OpenGL present-hook overlay for D2Debugger.
//
// WHY THIS EXISTS (see conformance/LIVE_DISPATCH_FRAMEWORK_PLAN.md Phase 4
// postmortem): the original D2Debugger (D2Debugger.imgui.d3d9.cpp) creates its
// OWN separate window + its OWN Direct3D9 device, and drives rendering from the
// D2Game GAME_UpdateProgress hook. Against real PD2 that fails three ways at
// once: (1) PD2 renders with OpenGL (d2gl/glide3x.dll or cnc-ddraw
// renderer=opengl -- both present via wglSwapBuffers/SwapBuffers), so a D3D9
// device can't share the window; (2) the separate debugger window is created
// non-topmost at the borderless-fullscreen game's position -> hidden behind it;
// (3) the GAME_UpdateProgress hook is at a D2Game ordinal whose RVA may be wrong
// for PD2-S12 (ordinals are scrambled -- see ORDINAL_RECONCILIATION.md), so the
// render loop may never run at all.
//
// This overlay sidesteps ALL THREE by using the industry-standard game-overlay
// technique: Detours-hook the game's OWN present call (wglSwapBuffers /
// SwapBuffers) and render Dear ImGui into the game's OWN GL context, in the
// game's OWN window, on the render thread -- no separate device, no separate
// window, no dependency on any game-logic hook. Confirmed via binary import
// analysis: glide3x.dll (d2gl) imports opengl32!wglSwapBuffers; ddraw.dll
// (cnc-ddraw, renderer=opengl) imports gdi32!SwapBuffers -- we hook both.
//
// Self-installing: a poll thread (started from DllMain) waits for opengl32 to
// load, then installs the Detours hooks. Independent of whether the D2Game/
// D2Client logic hooks fire.
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"

#include <Windows.h>
#include <GL/gl.h>       // core GL 1.1 (glGetIntegerv/GL_VIEWPORT) exported by opengl32
#include <tlhelp32.h>    // full module enumeration (kernel32, no extra link)
#include <detours.h>
#include <atomic>
#include <cstdio>

#include "D2Debugger.h"

// ImGui's Win32 message handler (defined in imgui_impl_win32.cpp).
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
	using SwapBuffersFn = BOOL(WINAPI*)(HDC);

	using SwapLayerFn = BOOL(WINAPI*)(HDC, UINT);
	using GrBufferSwapFn = void(__stdcall*)(unsigned int);

	SwapBuffersFn real_wglSwapBuffers = nullptr;   // opengl32.dll (d2gl path)
	SwapBuffersFn real_SwapBuffers = nullptr;      // gdi32.dll (cnc-ddraw path)
	SwapLayerFn real_wglSwapLayerBuffers = nullptr; // opengl32.dll (some GL apps)
	// glide3x.dll (d2gl) Glide-level present -- the D2 engine calls this every
	// frame in Glide mode. d2gl implements it as its GL render+present, so this
	// is the correct interception point when the raw GL swap is bypassed.
	GrBufferSwapFn real_grBufferSwap = nullptr;

	std::atomic<bool> g_glActive{ false };
	bool g_imguiInit = false;
	HWND g_hwnd = nullptr;
	WNDPROC g_origWndProc = nullptr;

	// wglSwapBuffers internally calls gdi32!SwapBuffers, so with BOTH hooked a
	// single present would render twice (and recurse). This per-thread depth
	// counter makes ONLY the outermost present of a frame render.
	thread_local int g_presentDepth = 0;

	// SPIKE diagnostic: a fixed log file so we can confirm the hook installed and
	// is firing even if nothing is visible on screen (Game.exe has no console).
	void OverlayLog(const char* msg)
	{
		FILE* f = nullptr;
		if (fopen_s(&f, "C:\\Users\\benam\\source\\cpp\\D2MOO\\conformance\\behavioral\\overlay_gl_log.txt", "a") == 0 && f)
		{
			fprintf(f, "%s\n", msg);
			fclose(f);
		}
	}

	LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;
		return CallWindowProcW(g_origWndProc, hWnd, msg, wParam, lParam);
	}

	void EnsureImGuiInit(HDC hdc)
	{
		if (g_imguiInit)
			return;
		g_hwnd = WindowFromDC(hdc);
		if (!g_hwnd)
			return;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::GetIO().IniFilename = nullptr; // don't write imgui.ini into the game dir
		ImGui::StyleColorsDark();

		ImGui_ImplWin32_Init(g_hwnd);
		ImGui_ImplOpenGL3_Init(nullptr); // auto-detect GLSL version; uses imgui's bundled GL loader

		// Subclass the game's window so ImGui receives input.
		g_origWndProc = (WNDPROC)SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)OverlayWndProc);
		g_imguiInit = true;

		char buf[128];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "EnsureImGuiInit OK: hwnd=%p glVer=%s", (void*)g_hwnd, (const char*)glGetString(GL_VERSION));
		OverlayLog(buf);
	}

	void RenderOverlay(HDC hdc)
	{
		EnsureImGuiInit(hdc);
		if (!g_imguiInit)
			return;

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplWin32_NewFrame();

		// Robustness: if the Win32 backend couldn't derive a valid display size
		// from the window (e.g. WindowFromDC returned a 0x0 / wrong surface under
		// cnc-ddraw), fall back to the actual GL viewport so ImGui doesn't cull
		// everything to a zero-size display.
		ImGuiIO& io = ImGui::GetIO();
		if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f)
		{
			GLint vp[4] = { 0, 0, 0, 0 };
			glGetIntegerv(GL_VIEWPORT, vp);
			if (vp[2] > 0 && vp[3] > 0)
				io.DisplaySize = ImVec2((float)vp[2], (float)vp[3]);
		}

		ImGui::NewFrame();

		// Always-on heartbeat window: immediate visual proof the overlay renders
		// over PD2 (independent of any game-state panel).
		if (ImGui::Begin("D2MOO Overlay"))
		{
			ImGui::Text("OpenGL present-hook overlay ACTIVE");
			ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		}
		ImGui::End();

		// The Phase 4 registry viewer (self-contained: reads registry.json,
		// needs no game state / no game-logic hook).
		D2DebugLiveDispatch();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	BOOL WINAPI Hooked_wglSwapBuffers(HDC hdc)
	{
		static bool logged = false;
		if (!logged) { logged = true; OverlayLog("Hooked_wglSwapBuffers FIRED"); }
		g_glActive.store(true, std::memory_order_relaxed);
		const bool outermost = (g_presentDepth++ == 0);
		if (outermost)
			RenderOverlay(hdc);
		const BOOL r = real_wglSwapBuffers(hdc);
		--g_presentDepth;
		return r;
	}

	BOOL WINAPI Hooked_SwapBuffers(HDC hdc)
	{
		static bool logged = false;
		if (!logged) { logged = true; OverlayLog("Hooked_SwapBuffers FIRED"); }
		g_glActive.store(true, std::memory_order_relaxed);
		const bool outermost = (g_presentDepth++ == 0);
		if (outermost)
			RenderOverlay(hdc);
		const BOOL r = real_SwapBuffers(hdc);
		--g_presentDepth;
		return r;
	}

	BOOL WINAPI Hooked_wglSwapLayerBuffers(HDC hdc, UINT planes)
	{
		static bool logged = false;
		if (!logged) { logged = true; OverlayLog("Hooked_wglSwapLayerBuffers FIRED"); }
		g_glActive.store(true, std::memory_order_relaxed);
		const bool outermost = (g_presentDepth++ == 0);
		if (outermost)
			RenderOverlay(hdc);
		const BOOL r = real_wglSwapLayerBuffers(hdc, planes);
		--g_presentDepth;
		return r;
	}

	// d2gl Glide present. The GL context is current here; render ImGui into the
	// current (default) framebuffer at entry, then let d2gl present it. Uses
	// wglGetCurrentDC() for the HWND since Glide doesn't hand us an HDC.
	void __stdcall Hooked_grBufferSwap(unsigned int swapInterval)
	{
		static bool logged = false;
		if (!logged) { logged = true; OverlayLog("Hooked_grBufferSwap FIRED"); }
		g_glActive.store(true, std::memory_order_relaxed);
		const bool outermost = (g_presentDepth++ == 0);
		if (outermost)
			RenderOverlay(wglGetCurrentDC());
		real_grBufferSwap(swapInterval);
		--g_presentDepth;
	}

	// Full module list -- so we can see EXACTLY what renderer is active (ANGLE
	// libGLESv2/libEGL? a d3d9/d3d11/dxgi swapchain? the vendor ICD nvoglv32/
	// atioglxx presenting directly?) rather than guessing from a filtered set.
	void LogAllModules()
	{
		OverlayLog("=== ALL loaded modules ===");
		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
		if (snap == INVALID_HANDLE_VALUE)
		{
			OverlayLog("  (snapshot failed)");
			return;
		}
		MODULEENTRY32W me{};
		me.dwSize = sizeof(me);
		if (Module32FirstW(snap, &me))
		{
			do
			{
				char nameA[MAX_PATH] = "";
				WideCharToMultiByte(CP_ACP, 0, me.szModule, -1, nameA, sizeof(nameA), nullptr, nullptr);
				char buf[MAX_PATH + 64];
				_snprintf_s(buf, sizeof(buf), _TRUNCATE, "  base=%p size=%lu %s",
					(void*)me.modBaseAddr, (unsigned long)me.modBaseSize, nameA);
				OverlayLog(buf);
			} while (Module32NextW(snap, &me));
		}
		CloseHandle(snap);
	}

	void InstallPresentHooks()
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		if (HMODULE hOpenGL = GetModuleHandleW(L"opengl32.dll"))
		{
			real_wglSwapBuffers = (SwapBuffersFn)GetProcAddress(hOpenGL, "wglSwapBuffers");
			if (real_wglSwapBuffers)
				DetourAttach(&(PVOID&)real_wglSwapBuffers, (PVOID)Hooked_wglSwapBuffers);
			real_wglSwapLayerBuffers = (SwapLayerFn)GetProcAddress(hOpenGL, "wglSwapLayerBuffers");
			if (real_wglSwapLayerBuffers)
				DetourAttach(&(PVOID&)real_wglSwapLayerBuffers, (PVOID)Hooked_wglSwapLayerBuffers);
		}
		if (HMODULE hGdi = GetModuleHandleW(L"gdi32.dll"))
		{
			real_SwapBuffers = (SwapBuffersFn)GetProcAddress(hGdi, "SwapBuffers");
			if (real_SwapBuffers)
				DetourAttach(&(PVOID&)real_SwapBuffers, (PVOID)Hooked_SwapBuffers);
		}
		// d2gl Glide present (the actual per-frame present in Glide mode).
		if (HMODULE hGlide = GetModuleHandleW(L"glide3x.dll"))
		{
			real_grBufferSwap = (GrBufferSwapFn)GetProcAddress(hGlide, "_grBufferSwap@4");
			if (real_grBufferSwap)
				DetourAttach(&(PVOID&)real_grBufferSwap, (PVOID)Hooked_grBufferSwap);
		}

		const LONG err = DetourTransactionCommit();
		char buf[200];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"InstallPresentHooks: commit=%ld wglSwapBuffers=%p SwapBuffers=%p grBufferSwap=%p",
			err, (void*)real_wglSwapBuffers, (void*)real_SwapBuffers, (void*)real_grBufferSwap);
		OverlayLog(buf);
		LogAllModules();
	}

	DWORD WINAPI InstallThread(LPVOID)
	{
		// Wait for the OpenGL renderer to load (d2gl/cnc-ddraw pull in opengl32
		// once the game initializes graphics). Bounded so vanilla D2 (which may
		// never load opengl32 -- it uses the D3D9 path) doesn't spin forever.
		OverlayLog("InstallThread: started, waiting for opengl32.dll...");
		for (int i = 0; i < 600; ++i) // ~30s at 50ms
		{
			if (GetModuleHandleW(L"opengl32.dll"))
			{
				OverlayLog("InstallThread: opengl32.dll found, installing hooks");
				InstallPresentHooks();
				return 0;
			}
			Sleep(50);
		}
		OverlayLog("InstallThread: TIMED OUT waiting for opengl32.dll (vanilla D3D9 path?)");
		return 0;
	}
}

// Called from DllMain(DLL_PROCESS_ATTACH). Starts the self-installing poll
// thread. Independent of the game-logic hooks, so it works even when the
// D2Game GAME_UpdateProgress hook doesn't fire on PD2.
void D2Overlay_GL_StartInstall()
{
	if (HANDLE h = CreateThread(nullptr, 0, InstallThread, nullptr, 0, nullptr))
		CloseHandle(h);
}

// True once the OpenGL present hook has fired at least once -- the D3D9 path
// (D2Debugger.patch.common.cpp) defers to us when this is set, so the two
// never fight over the single global ImGui context.
bool D2Overlay_IsGLActive()
{
	return g_glActive.load(std::memory_order_relaxed);
}
