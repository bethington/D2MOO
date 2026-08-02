// This is basically a modified copy of the Dear ImGui standalone example application for DirectX 9
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include "IconsFontAwesome6.h"
#include "fa-solid-900.cpp" // NOLINT

#include "D2Debugger.h"
#include "D2Debugger.theme.h"

// Data
struct DebuggerData
{
    LPDIRECT3D9              pD3D = nullptr;
    LPDIRECT3DDEVICE9        pd3dDevice = nullptr;
    D3DPRESENT_PARAMETERS    d3dpp = {};
    HWND                     hWindow = nullptr;
    WNDCLASSEXW              windowClassEx;
    bool                     bShowDemo = false; // off by default; toggle via the debug menu if needed
} gD2DebuggerData;

// The live D3D9 device, for code that needs to create its own resources on it
// (D2Debugger.gamepanel.cpp uploads game frames into a texture). Returns null
// before init and after teardown, so callers must check every frame rather than
// caching it -- a device Reset invalidates D3DPOOL_DEFAULT resources.
LPDIRECT3DDEVICE9 D2Panel_GetDevice() { return gD2DebuggerData.pd3dDevice; }
// The host window the ImGui panels live inside. A panel cannot be wider than
// its viewport, so a 1:1 lock has to be able to grow this.
HWND D2Panel_GetHostWindow() { return gD2DebuggerData.hWindow; }

// Forward declarations of helper functions
void D2DebugGamePanel_RegisterSettings();   // must run before the first frame
void D2Host_RegisterSettings();             // ditto -- [D2Host] block
void D2Snap_RegisterSettings();             // ditto -- [D2PanelSnap] block
void D2Snap_NewFrame();                     // magnetic panel snapping, per frame
// The debugger's own window: monitor choice, borderless fill, DPI (see
// D2Debugger.hostwindow.cpp).
extern "C" void  D2Host_PreloadSettings(const char* iniPath);
extern "C" void  D2Host_ThreadDpiAware();
extern "C" void  D2Host_ApplyStartupLayout(HWND hwnd);
extern "C" void  D2Host_ToggleFill(HWND hwnd);
extern "C" void  D2Host_OnDpiChanged(HWND hwnd, WPARAM wParam, LPARAM lParam);
extern "C" float D2Host_RefreshDpiScale(HWND hwnd);
extern "C" float D2Host_DpiScale();
extern "C" void  D2Host_Tick(HWND hwnd);            // re-asserts the fill if it drifts
extern "C" void  D2Host_NoteSize(int w, int h);     // logs every WM_SIZE
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND GetGameWindow()
{
    HWND hGameWindow = nullptr;
    EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
        {
            DWORD dwProcessId = 0x0;
            GetWindowThreadProcessId(hWnd, &dwProcessId);
            // Only consider the current process windows.
            if (GetCurrentProcessId() == dwProcessId)
            {
                char buf[2048];
                // There are many windows that may be created due to compatibility layers...
                // So sadly we need to filter by name too.
                // An alternative would be to get the window handle directly from D2Client.dll
                if (GetWindowTextA(hWnd, buf, _countof(buf)) && strstr(buf, "Diablo"))
                {
                    *(HWND*)lParam = hWnd;
                    SetLastError(ERROR_SUCCESS);
                    // Stop iteration here.
                    return FALSE;
                }
            }
            return TRUE;
        }
    , (LPARAM)&hGameWindow);
    return hGameWindow;
}

void GetWindowPositionFromGameWindow(int& x, int& y)
{
    const HWND hGameWindow = GetGameWindow();
    if (!hGameWindow)
    {
        x = CW_USEDEFAULT;
        return;
    }
    MONITORINFO monitorInfo = { 0 };
    monitorInfo.cbSize = sizeof(MONITORINFO);
    GetMonitorInfoA(MonitorFromWindow(hGameWindow, MONITOR_DEFAULTTOPRIMARY), &monitorInfo);

    RECT windowRect;
    GetWindowRect(hGameWindow, &windowRect);

    if (windowRect.left == monitorInfo.rcMonitor.left
        && windowRect.right == monitorInfo.rcMonitor.right
        && windowRect.top == monitorInfo.rcMonitor.top
        && windowRect.bottom == monitorInfo.rcMonitor.bottom)
    {
        // Fullscreen, same position as game window
        x = windowRect.left;
        y = windowRect.top;
    }
    else
    {
        // Windowed, on the right of the game window
        x = windowRect.right + 5;
        y = windowRect.top;
    }
}

bool IsCursorVisible()
{
    CURSORINFO ci = { sizeof(CURSORINFO) };
    if (GetCursorInfo(&ci))
        return ci.flags & CURSOR_SHOWING;
    return false;
}

// Keep ImGui's own metrics in step with the monitor scale.
//
// Cumulative by design: ScaleAllSizes multiplies the CURRENT values, so it must
// be fed the RATIO against whatever is already applied, never the absolute
// scale -- calling it twice with 1.25 gives you 1.5625 and a UI that grows
// every time the window changes monitor.
static float gUiScaleApplied = 1.0f;
static void ApplyUiScale(float scale)
{
    if (scale <= 0.0f || scale == gUiScaleApplied)
        return;
    ImGui::GetStyle().ScaleAllSizes(scale / gUiScaleApplied);
    // Fonts are scaled separately (they are not "sizes"): 1.92's dynamic font
    // system reads FontScaleDpi every frame, so this needs no atlas rebuild.
    ImGui::GetStyle().FontScaleDpi = scale;
    gUiScaleApplied = scale;
}

D2DEBUGGER_DLL_DECL
int D2DebuggerInit()
{
    // Create application window.
    //
    // DPI FIRST, before the window exists -- a window inherits the awareness of
    // the thread that creates it. This is thread-scoped (see
    // D2Debugger.hostwindow.cpp); ImGui_ImplWin32_EnableDpiAwareness() is NOT
    // used because its 8.1 fallback goes process-wide, and process-wide
    // awareness inside Game.exe would silently re-base the coordinate space of
    // the game's own window and of everything driving it from outside.
    D2Host_PreloadSettings(nullptr);   // "imgui.ini" in the cwd, i.e. beside Game.exe
    D2Host_ThreadDpiAware();
    gD2DebuggerData.windowClassEx = { sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"D2Debugger", nullptr };
    ::RegisterClassExW(&gD2DebuggerData.windowClassEx);
    int x = 0, y = 0;
    GetWindowPositionFromGameWindow(x, y);
    gD2DebuggerData.hWindow = ::CreateWindowW(gD2DebuggerData.windowClassEx.lpszClassName, L"D2Debugger", WS_OVERLAPPEDWINDOW, x, y, 1000, 800, nullptr, nullptr, gD2DebuggerData.windowClassEx.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(gD2DebuggerData.hWindow))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(gD2DebuggerData.windowClassEx.lpszClassName, gD2DebuggerData.windowClassEx.hInstance);
        return 1;
    }

    // Show the window, then immediately put it where it belongs -- borderless,
    // filling its monitor. Done here rather than after the render loop starts so
    // there is no frame at the old hardcoded 900x720 to flash past.
    ::ShowWindow(gD2DebuggerData.hWindow, SW_SHOWDEFAULT);
    ::UpdateWindow(gD2DebuggerData.hWindow);
    D2Host_ApplyStartupLayout(gD2DebuggerData.hWindow);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style -- dark, with hints of Diablo II
    // (D2Debugger.theme.cpp). Before ApplyUiScale below, which multiplies the
    // theme's base spacing/rounding for the monitor's DPI.
    D2Theme_Apply();
    // Real pixels means SMALL pixels: per-monitor aware on a 125% display, a
    // 13px font is 13 actual pixels rather than the 16 the operator is used to.
    // Scale the UI back up ourselves -- crisply, unlike DWM stretching a
    // virtualized surface, and without touching the game panel's image, which
    // is drawn at texture pixels either way and stays exactly 1:1.
    ApplyUiScale(D2Host_RefreshDpiScale(gD2DebuggerData.hWindow));

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(gD2DebuggerData.hWindow);
    // Before the first NewFrame: that is when imgui.ini is parsed, so a
    // handler added later would never see the saved values.
    D2DebugGamePanel_RegisterSettings();
    D2Host_RegisterSettings();
    D2Snap_RegisterSettings();
    ImGui_ImplDX9_Init(gD2DebuggerData.pd3dDevice);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    float baseFontSize = 13.0f; // 13.0f is the size of the default font. Change to the font size you use.
    float iconFontSize = baseFontSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

    // merge in icons from Font Awesome
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;
    ImFont* fontAwesome = io.Fonts->AddFontFromMemoryCompressedTTF(
        faSolid900_compressed_compressed_data, faSolid900_compressed_compressed_size, 
        iconFontSize, &icons_config, icons_ranges
    );
    IM_ASSERT(fontAwesome != nullptr);
    return 0;
}

D2DEBUGGER_DLL_DECL
void D2DebugGamePanel();
void D2DebugGamePanel_Shutdown();
extern "C" void D2GameWindow_Restore();
void D2DebugGamePanel_ReleaseDeviceObjects();

void D2DebuggerDestroy()
{
    // Put the game window back before we go: parked off-screen with the
    // debugger gone, nothing is left that could restore it.
    D2GameWindow_Restore();
    // Before anything releases the device: a D3DPOOL_DEFAULT texture that
    // outlives its device is exactly what makes a later Reset fail.
    D2DebugGamePanel_Shutdown();
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(gD2DebuggerData.hWindow);
    ::UnregisterClassW(gD2DebuggerData.windowClassEx.lpszClassName, gD2DebuggerData.windowClassEx.hInstance);
}

// returns true if should quit
D2DEBUGGER_DLL_DECL
bool D2DebuggerNewFrame()
{
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    // Poll and handle messages (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    while (::PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);

        if (msg.message == WM_QUIT)
        {
            return true;
        }
    }

    // Some rendering backends seem to force hide all cursors of the application.
    // So detect this case and ask ImGui to render it instead.
    if (!IsCursorVisible())
    {
        auto& io = ImGui::GetIO();
        io.MouseDrawCursor = true;
    }
    // Start the Dear ImGui frame
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (gD2DebuggerData.bShowDemo)
        ImGui::ShowDemoWindow(&gD2DebuggerData.bShowDemo);

    return false;
}

// ---------------------------------------------------------------------------
// Standalone debugger window, driven by its OWN thread + its OWN D3D9 device.
//
// WHY (see conformance/LIVE_DISPATCH_FRAMEWORK_PLAN.md Phase 4): against PD2 the
// present-hook overlay is infeasible -- PD2's render stack (d2gl + cnc-ddraw +
// SGD2FreeDisplayFix + the AcLayers compat shim) presents through a path that
// bypasses wglSwapBuffers / SwapBuffers / wglSwapLayerBuffers / grBufferSwap
// (all confirmed hooked-but-never-firing). So instead of drawing INTO the game's
// renderer, we run a SEPARATE top-most window with our own D3D9 device --
// renderer-agnostic, works no matter how the game presents. This also fixes the
// two reasons the original D2Debugger never showed on PD2: it was pumped only
// from the (dead-on-PD2) GAME_UpdateProgress hook, and positioned BEHIND the
// borderless-fullscreen game. Here we drive it from a dedicated thread and make
// it top-most + visible.
static volatile bool g_standaloneRunning = false;

bool D2Debugger_IsStandaloneActive() { return g_standaloneRunning; }

void D2Mcp_StartServer(); // WS-5 MCP control server (D2Debugger.mcp.cpp)
void D2Capture_Init();     // live game-object handle capture (D2Debugger.capture.cpp)
extern "C" void D2Action_InstallPumpHook(); // pre-game D2Win menu pump site (D2Debugger.action.cpp)
extern "C" void D2Asset_InstallServerGameHook(); // server-Game* capture for /showcase/item (D2Debugger.assetreload.cpp)
extern "C" void D2Asset_InstallEarlyRegHook();   // pre-table-load overlay auto-registration (D2Debugger.assetreload.cpp)
extern "C" void D2Crash_Install();               // fault observer (D2Debugger.crash.cpp)
extern "C" void D2AudioCap_Install();            // DirectSound capture + mixer (D2Debugger.audiocap.cpp)
extern "C" void D2AudioStream_Start();           // FLAC/WebSocket audio stream (D2Debugger.audiostream.cpp)

static DWORD WINAPI StandaloneThread(LPVOID)
{
    if (D2DebuggerInit() != 0)
        return 1;

    // WS-5: bring up the localhost HTTP control surface so an external agent can
    // drive shadow-proving. Independent of the render loop; safe if it fails.
    // Before anything else that could fault: a crash during startup is exactly
    // the case with no other witness.
    D2Crash_Install();
    D2AudioCap_Install();
    // After the capture: the streamer consumes the ring the mixer fills, and
    // idles at zero cost until a client actually connects.
    D2AudioStream_Start();
    D2Mcp_StartServer();
    // Stateful frontier: attach the live game-object handle capture hook.
    D2Capture_Init();
    // Pre-game pump site: D2Capture's hook only fires in-world, but actions
    // like entering single-player must run BEFORE any game exists.
    D2Action_InstallPumpHook();
    // Asset Studio: capture the SERVER Game* from the per-frame server tick so
    // /showcase/item can spawn items server-side (see D2Debugger.assetreload.cpp).
    D2Asset_InstallServerGameHook();
    // Retry the early-registration hook in case D2Common wasn't loaded yet when
    // DllMain ran (idempotent; normally already installed by StartStandalone).
    D2Asset_InstallEarlyRegHook();

    // Top-most, and filling its monitor (the game may be borderless-fullscreen,
    // so a non-topmost window at the game's rect would hide behind it). The
    // geometry itself is owned by D2Host_ApplyStartupLayout, called from
    // D2DebuggerInit above -- there is deliberately no second placement here to
    // disagree with it.
    ::SetWindowPos(gD2DebuggerData.hWindow, HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    g_standaloneRunning = true;

    while (g_standaloneRunning)
    {
        if (D2DebuggerNewFrame()) // pumps messages, starts the ImGui frame (+demo)
            break;
        // Put the host back if anything moved or resized it out from under us.
        // Before the snapping, so the panels are pinned against the corrected
        // client area rather than one frame behind it.
        D2Host_Tick(gD2DebuggerData.hWindow);
        // BEFORE the panels are drawn: a window's Begin() uses the position it
        // currently has, so re-pinning after the fact would show the previous
        // position for a frame on every snap.
        D2Snap_NewFrame();        // magnetic 9-point snapping + remembered anchors
        D2DebugLiveDispatch();    // unified function browser (profiler tree + dispatch control)
        D2DebugGamePanel();       // the game itself, as a movable panel
        D2DebuggerEndFrame(true);
        // NOTE: the game-thread call queue is deliberately NOT pumped here --
        // this loop runs on D2Debugger's OWN thread, and stateful UI calls
        // (e.g. the menu launch action) must run on the GAME thread. The queue
        // is drained by game-thread hooks only: D2Capture (in-world) and the
        // D2Win RenderMainFrame menu hook (D2Debugger.action.cpp).
        ::Sleep(16);
    }
    return 0;
}

// Started from DllMain -- independent of the game-logic hooks and the game's
// renderer.
void D2Debugger_StartStandalone()
{
    static bool started = false;
    if (started)
        return;
    started = true;
    // Must be in place BEFORE the game's startup sequence reaches data-table load
    // (which happens pre-menu), so install synchronously here in DllMain context
    // rather than on the standalone thread. The detour only attaches a hook; its
    // body runs later on the game's own data-load thread.
    D2Asset_InstallEarlyRegHook();
    if (HANDLE h = ::CreateThread(nullptr, 0, StandaloneThread, nullptr, 0, nullptr))
        ::CloseHandle(h);
}

D2DEBUGGER_DLL_DECL
void D2DebuggerEndFrame(bool VSyncNextFrame)
{
    // Rendering
    ImGui::EndFrame();
    gD2DebuggerData.pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    gD2DebuggerData.pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    gD2DebuggerData.pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    
    // What you see where no panel is. It was a pale blue-grey, which on a
    // 2560x1440 borderless surface is by AREA the most visible colour in the
    // application -- and the least Diablo. Owned by the theme now.
    float clear_color[4];
    D2Theme_ClearColor(clear_color);
    D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color[0] * 255.0f), (int)(clear_color[1] * 255.0f), (int)(clear_color[2] * 255.0f), (int)(clear_color[3] * 255.0f));
    gD2DebuggerData.pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
    if (gD2DebuggerData.pd3dDevice->BeginScene() >= 0)
    {
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        gD2DebuggerData.pd3dDevice->EndScene();
    }
    HRESULT result = gD2DebuggerData.pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

    // Handle loss of D3D9 device
    const UINT NextFramePresentationInterval = VSyncNextFrame ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
    if ((NextFramePresentationInterval != gD2DebuggerData.d3dpp.PresentationInterval) ||
        (result == D3DERR_DEVICELOST && gD2DebuggerData.pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET))
    {
        gD2DebuggerData.d3dpp.PresentationInterval = NextFramePresentationInterval;
        ResetDevice();
    }
}



// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    gD2DebuggerData.pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (gD2DebuggerData.pD3D == nullptr)
        return false;

    // Create the D3DDevice
    ZeroMemory(&gD2DebuggerData.d3dpp, sizeof(gD2DebuggerData.d3dpp));
    gD2DebuggerData.d3dpp.Windowed = TRUE;
    gD2DebuggerData.d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    gD2DebuggerData.d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    gD2DebuggerData.d3dpp.EnableAutoDepthStencil = TRUE;
    gD2DebuggerData.d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    //gD2DebuggerData.d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    gD2DebuggerData.d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (gD2DebuggerData.pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &gD2DebuggerData.d3dpp, &gD2DebuggerData.pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (gD2DebuggerData.pd3dDevice) { gD2DebuggerData.pd3dDevice->Release(); gD2DebuggerData.pd3dDevice = nullptr; }
    if (gD2DebuggerData.pD3D) { gD2DebuggerData.pD3D->Release(); gD2DebuggerData.pD3D = nullptr; }
}

void ResetDevice()
{
    // The game-frame texture is D3DPOOL_DEFAULT; it MUST go before Reset or the
    // Reset fails and the host asserts.
    D2DebugGamePanel_ReleaseDeviceObjects();
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = gD2DebuggerData.pd3dDevice->Reset(&gD2DebuggerData.d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        // Logged unconditionally. This window is resized by code the operator
        // cannot see, and not knowing WHICH resize was the bad one is exactly
        // what made a borderless host stuck at 1112x1380 hard to explain.
        D2Host_NoteSize(LOWORD(lParam), HIWORD(lParam));
        if (gD2DebuggerData.pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            gD2DebuggerData.d3dpp.BackBufferWidth = LOWORD(lParam);
            gD2DebuggerData.d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        // F11 toggles borderless-fills-the-monitor. Handled at the Win32 level
        // rather than in a panel because borderless removes the title bar and
        // the sysmenu -- with no key there would be nothing left to grab, and a
        // window that cannot be un-fullscreened is a window you have to kill the
        // game to escape. Not forwarded to the game: F11 is not in the panel's
        // key table (D2Debugger.gamepanel.cpp kKeys).
        if (wParam == VK_F11)
        {
            D2Host_ToggleFill(hWnd);
            return 0;
        }
        break;
    case WM_DPICHANGED:
        // Only reachable while windowed (dragged to a monitor with another
        // scale), but reachable. Take the rect Windows suggests, then bring
        // ImGui's own metrics along or the UI keeps the old monitor's size.
        D2Host_OnDpiChanged(hWnd, wParam, lParam);
        if (ImGui::GetCurrentContext())
            ApplyUiScale(D2Host_DpiScale());
        return 0;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
	default:
		break;
    }
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
