// D2Debugger.gamepanel.cpp -- the game as a movable ImGui panel.
//
// Phase 3 of "game as an ImGui panel": Phase 1 made input virtual
// (D2Debugger.vinput.cpp), Phase 2 found the real present point and made frame
// capture clean and correctly oriented (D2Debugger.vcapture.cpp). This draws
// those frames into a window you can move, resize and click into.
//
// WHY THIS IS NOT A COPY-PER-FRAME PIPELINE. The capture layer double-buffers:
// the game's render thread expands the 8bpp palette into the back buffer with
// no lock held, then swaps. Here we lock the front buffer and upload it
// STRAIGHT into a D3D9 texture -- no staging vector, no intermediate frame
// object. One expansion on the game thread, one upload on this one.
//
// Streaming is enabled only while the panel is actually open. The expansion is
// ~2.5 MB per frame at the in-world resolution, and paying that when nothing is
// looking is pure waste on the game's own render thread.
//
// INPUT ROUTING. Panel-local mouse position is mapped back to GAME coordinates
// and pushed through the virtual-input layer, which posts WM_MOUSEMOVE
// in-process. That indirection is not optional: the game runs elevated, so the
// same PostMessage from an operator shell is dropped silently by UIPI, and D2
// takes its cursor position from the message queue rather than GetCursorPos
// (measured -- see D2Debugger.vinput.cpp).

#include "imgui.h"
// ImGuiSettingsHandler / MarkIniSettingsDirty are internal API. Using them is
// the supported way to keep custom state in imgui.ini -- the alternative is a
// second settings file that can disagree with the window layout after a crash.
#include "imgui_internal.h"
#include <d3d9.h>
#include <windows.h>
#include <algorithm>
#include <cfloat>

// ---- capture (D2Debugger.vcapture.cpp) --------------------------------------
extern "C" void D2Capture_StreamEnable(int on);
extern "C" int  D2Capture_StreamEnabled();
extern "C" unsigned long D2Capture_StreamSeq();
extern "C" const unsigned char* D2Capture_LockFrame(int* w, int* h, unsigned long* seq);
extern "C" void D2Capture_UnlockFrame();

// ---- virtual input (D2Debugger.vinput.cpp) ----------------------------------
extern "C" void D2VInput_SetEnabled(int on);
extern "C" int  D2VInput_IsEnabled();
extern "C" int  D2VInput_MoveToGameXY(int gameX, int gameY, int* outX, int* outY);
extern "C" void D2VInput_SetKey(int vk, int down);
extern "C" int  D2VInput_PostMouseButton(int button, int down, int clientX, int clientY);
extern "C" int  D2VInput_PostKey(int vk, int down);
extern "C" int  D2VInput_ClipCursorReal(const void* rect);
extern "C" int  D2GameWindow_SetMode(int mode);
extern "C" int  D2GameWindow_Mode();
extern "C" void D2AudioCap_SetPlayLocal(int on);
extern "C" int  D2AudioCap_PlayLocal();

// The debugger's own D3D9 device, owned by D2Debugger.imgui.d3d9.cpp.
LPDIRECT3DDEVICE9 D2Panel_GetDevice();
HWND D2Panel_GetHostWindow();

namespace
{
	LPDIRECT3DTEXTURE9 g_tex = nullptr;
	int g_texW = 0, g_texH = 0;
	unsigned long g_lastSeq = 0;
	bool g_open = true;
	bool g_routeInput = true;
	// Hide the OS cursor over the image so the game's OWN rendered cursor is the
	// only one. Without it you get two: D2 draws its cursor into the frame we
	// capture, and Windows paints its arrow on top.
	bool g_hideCursor = true;
	// Lock the window to the frame's own size, 1:1 with no scaling.
	//
	// The render resolution is NOT constant -- measured 800x600 at the menu and
	// 1068x600 in-world in one session -- so this follows whatever the game is
	// currently producing rather than pinning a number. Entering or leaving a
	// game therefore resizes the window, which is the price of never scaling.
	// DEFAULTS ON: the panel is how the game is meant to be used now, so it
	// starts pixel-exact with the real window out of the way. The persisted
	// settings below override these whenever a previous session left any.
	bool g_lockNative = true;
	bool g_wantHideGame = true;
	bool g_wantAudio = false;
	// CURSOR CAPTURE. Off by default -- confining the operator's pointer
	// without being asked is hostile, and this is an explicit 'playing now' mode.
	// Virtual input, PERSISTED. It is runtime state in the vinput layer and
	// resets to off on every game restart -- which silently disabled the
	// cursor hide (which is gated on it) after each relaunch, with the
	// checkbox still showing ticked. Every other option here was remembered;
	// this one was not, so it looked like the cursor hide had broken.
	bool g_wantVirtual = false;
	bool g_lockCursor = false;
	// Keep the pointer out of the bottom HUD strip while captured, so a
	// mis-aimed click cannot open the belt and drink a potion mid-fight.
	// Set by SHIFT-clicking Capture.
	bool g_hudGuard = false;
	// Height of D2's control panel in SOURCE pixels. Measured off a live
	// 1068x600 frame: centre-column brightness jumps from ~23 (world floor)
	// to ~45 at y=550 and ~90 by y=560, so the panel's top edge is ~548.
	// Expressed in source pixels rather than a fraction of height because
	// D2's panel is a fixed-height sprite anchored to the bottom -- it does
	// not scale with the window.
	constexpr int kHudGuardPx = 52;
	bool g_captured = false;        // clip currently applied (runtime only)
	// Ctrl+Alt was used to break out. Stays released until the image is clicked
	// again, or the clip would snap back the instant the keys came up.
	bool g_brokeOut = false;
	bool g_bootApplied = false;
	// Window chrome measured LAST frame: title bar + control row + padding.
	// SetNextWindowSize has to run before Begin, so the size needed cannot be
	// known until the row has been laid out once. Converges in a single frame
	// and then stays put, since the chrome height is constant.
	float g_chromeTop = 0.0f;
	float g_chromeSide = 0.0f;
	// Frames the texture actually took, so a stalled panel is visible as a
	// stalled number rather than a still image you might read as a paused game.
	unsigned long g_uploads = 0;

	// ---- persisted settings ------------------------------------------------
	//
	// Stored in imgui.ini beside the window's own position and size, which is
	// where this panel's layout already lives. ImGui saves it automatically;
	// MarkIniSettingsDirty on change is the whole contract. The file now sits
	// next to Game.exe since the launcher sets the working directory -- before
	// that fix it landed in whatever directory happened to launch the game.
	void SettingsClearAll(ImGuiContext*, ImGuiSettingsHandler*) {}

	void* SettingsReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char*)
	{
		return (void*)(intptr_t)1;      // a single anonymous entry
	}

	void SettingsReadLine(ImGuiContext*, ImGuiSettingsHandler*, void*, const char* line)
	{
		int v = 0;
		if      (sscanf_s(line, "Input=%d", &v) == 1)      g_routeInput = v != 0;
		else if (sscanf_s(line, "HideCursor=%d", &v) == 1) g_hideCursor = v != 0;
		else if (sscanf_s(line, "Lock11=%d", &v) == 1)     g_lockNative = v != 0;
		else if (sscanf_s(line, "HideGame=%d", &v) == 1)   g_wantHideGame = v != 0;
		else if (sscanf_s(line, "Audio=%d", &v) == 1)      g_wantAudio = v != 0;
		else if (sscanf_s(line, "Capture=%d", &v) == 1)    g_lockCursor = v != 0;
		else if (sscanf_s(line, "HudGuard=%d", &v) == 1)   g_hudGuard = v != 0;
		else if (sscanf_s(line, "Virtual=%d", &v) == 1)    g_wantVirtual = v != 0;
	}

	void SettingsWriteAll(ImGuiContext*, ImGuiSettingsHandler* h, ImGuiTextBuffer* buf)
	{
		buf->appendf("[%s][Settings]\n", h->TypeName);
		buf->appendf("Input=%d\n",      g_routeInput ? 1 : 0);
		buf->appendf("HideCursor=%d\n", g_hideCursor ? 1 : 0);
		buf->appendf("Lock11=%d\n",     g_lockNative ? 1 : 0);
		buf->appendf("HideGame=%d\n",   g_wantHideGame ? 1 : 0);
		buf->appendf("Audio=%d\n",      g_wantAudio ? 1 : 0);
		buf->appendf("Capture=%d\n",    g_lockCursor ? 1 : 0);
		buf->appendf("HudGuard=%d\n",   g_hudGuard ? 1 : 0);
		buf->appendf("Virtual=%d\n",    g_wantVirtual ? 1 : 0);
		buf->append("\n");
	}

	// Every option goes through this: one short label, one tooltip, one dirty
	// mark. Uniform by construction rather than by remembering to match.
	bool Opt(const char* label, bool* v, const char* help)
	{
		const bool changed = ImGui::Checkbox(label, v);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", help);
		if (changed)
			ImGui::MarkIniSettingsDirty();
		ImGui::SameLine();
		return changed;
	}

	void ReleaseTexture()
	{
		if (g_tex) { g_tex->Release(); g_tex = nullptr; }
		g_texW = g_texH = 0;
	}

	// Upload the newest frame if there is one. Returns false when nothing has
	// been captured yet (game not presenting, or streaming just enabled).
	bool SyncTexture()
	{
		LPDIRECT3DDEVICE9 dev = D2Panel_GetDevice();
		if (!dev)
			return false;
		if (D2Capture_StreamSeq() == g_lastSeq && g_tex)
			return true;                     // nothing new; keep showing the last frame

		int w = 0, h = 0;
		unsigned long seq = 0;
		const unsigned char* src = D2Capture_LockFrame(&w, &h, &seq);
		if (!src)
			return false;      // NOT holding the lock -- see the contract below

		bool ok = false;
		// Everything between here and Unlock blocks the game's render thread on
		// its next swap, so it is strictly: (re)create if the size changed, then
		// one row-wise copy. No format conversion -- the capture layer already
		// produced exactly what D3D9 wants, modulo channel order.
		if (w != g_texW || h != g_texH)
		{
			ReleaseTexture();
			if (SUCCEEDED(dev->CreateTexture((UINT)w, (UINT)h, 1, D3DUSAGE_DYNAMIC,
			                                 D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
			                                 &g_tex, nullptr)))
			{
				g_texW = w; g_texH = h;
			}
		}
		if (g_tex)
		{
			D3DLOCKED_RECT lr{};
			if (SUCCEEDED(g_tex->LockRect(0, &lr, nullptr, D3DLOCK_DISCARD)))
			{
				for (int y = 0; y < h; ++y)
				{
					const unsigned char* s = src + (size_t)y * w * 4u;
					unsigned char* d = (unsigned char*)lr.pBits + (size_t)y * lr.Pitch;
					// Capture gives R,G,B,A; D3DFMT_A8R8G8B8 is B,G,R,A in memory.
					for (int x = 0; x < w; ++x, s += 4, d += 4)
					{
						d[0] = s[2];
						d[1] = s[1];
						d[2] = s[0];
						d[3] = 255;
					}
				}
				g_tex->UnlockRect(0);
				ok = true;
			}
		}
		D2Capture_UnlockFrame();

		if (ok)
		{
			g_lastSeq = seq;
			++g_uploads;
		}
		return ok;
	}

	// Map a position inside the drawn image back to GAME pixels and push it
	// through virtual input. `img` is the top-left of the image in screen space.
	void RouteMouse(const ImVec2& img, const ImVec2& drawn)
	{
		if (!g_routeInput || g_texW <= 0 || g_texH <= 0)
			return;
		ImGuiIO& io = ImGui::GetIO();
		const float fx = (io.MousePos.x - img.x) / (drawn.x > 0 ? drawn.x : 1.0f);
		const float fy = (io.MousePos.y - img.y) / (drawn.y > 0 ? drawn.y : 1.0f);
		if (fx < 0.0f || fx > 1.0f || fy < 0.0f || fy > 1.0f)
			return;                          // outside the image: not ours to route

		const int gx = (int)(fx * (float)g_texW);
		const int gy = (int)(fy * (float)g_texH);
		D2VInput_MoveToGameXY(gx, gy, nullptr, nullptr);

		// Buttons go as real WM_*BUTTONDOWN/UP messages on the TRANSITION only.
		// Setting the async-key state every frame was not enough on its own --
		// measured live, the cursor tracked perfectly and clicks did nothing,
		// because D2 takes clicks from the message queue just like position.
		// Edge-triggered, so held buttons are one down + one up rather than a
		// stream of downs, which is what hold-to-attack expects.
		struct Btn { ImGuiMouseButton im; int vk; };
		static const Btn kBtns[] = {
			{ImGuiMouseButton_Left,  VK_LBUTTON},
			{ImGuiMouseButton_Right, VK_RBUTTON},
		};
		for (const Btn& b : kBtns)
		{
			if (ImGui::IsMouseClicked(b.im))
				D2VInput_PostMouseButton(b.vk, 1, gx, gy);
			else if (ImGui::IsMouseReleased(b.im))
				D2VInput_PostMouseButton(b.vk, 0, gx, gy);
		}
	}

	// The keys D2 actually reads on the hot path: skills, belt, UI toggles.
	// An EXPLICIT pairing rather than the Win32 backend's VK->ImGuiKey helper --
	// that function is a backend internal, and a fixed list also stops the panel
	// forwarding the debugger's own shortcuts into the game.
	struct KeyPair { ImGuiKey key; int vk; };
	const KeyPair kKeys[] = {
		{ImGuiKey_A,'A'},{ImGuiKey_S,'S'},{ImGuiKey_D,'D'},{ImGuiKey_W,'W'},
		{ImGuiKey_Q,'Q'},{ImGuiKey_E,'E'},{ImGuiKey_R,'R'},{ImGuiKey_F,'F'},
		{ImGuiKey_G,'G'},{ImGuiKey_T,'T'},{ImGuiKey_I,'I'},{ImGuiKey_C,'C'},
		{ImGuiKey_M,'M'},{ImGuiKey_B,'B'},{ImGuiKey_P,'P'},{ImGuiKey_V,'V'},
		{ImGuiKey_Z,'Z'},{ImGuiKey_X,'X'},
		{ImGuiKey_1,'1'},{ImGuiKey_2,'2'},{ImGuiKey_3,'3'},{ImGuiKey_4,'4'},
		{ImGuiKey_5,'5'},{ImGuiKey_6,'6'},{ImGuiKey_7,'7'},{ImGuiKey_8,'8'},
		{ImGuiKey_9,'9'},{ImGuiKey_0,'0'},
		{ImGuiKey_Escape,VK_ESCAPE},{ImGuiKey_Enter,VK_RETURN},
		{ImGuiKey_Space,VK_SPACE},{ImGuiKey_Tab,VK_TAB},
		{ImGuiKey_LeftShift,VK_SHIFT},{ImGuiKey_LeftCtrl,VK_CONTROL},
		{ImGuiKey_LeftAlt,VK_MENU},
		{ImGuiKey_F1,VK_F1},{ImGuiKey_F2,VK_F2},{ImGuiKey_F3,VK_F3},{ImGuiKey_F4,VK_F4},
		{ImGuiKey_F5,VK_F5},{ImGuiKey_F6,VK_F6},{ImGuiKey_F7,VK_F7},{ImGuiKey_F8,VK_F8},
	};

	// Is the break-out combo held? Also used to WITHHOLD those keys from the
	// game: otherwise every release of the cursor leaks a Ctrl and an Alt into
	// D2, and Alt pops the item labels, which reads as a glitch.
	bool BreakoutHeld()
	{
		return ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyDown(ImGuiKey_LeftAlt);
	}

	void RouteKeyboard()
	{
		if (!g_routeInput)
			return;
		const bool breakout = BreakoutHeld();
		// TRANSITIONS, posted as real messages. Re-asserting the polled state
		// every frame was never enough on its own -- exactly the failure the
		// mouse buttons had, where the state was right and nothing happened.
		for (const KeyPair& k : kKeys)
		{
			// The break-out combo belongs to the panel, not to the game.
			if (breakout && (k.vk == VK_CONTROL || k.vk == VK_MENU))
				continue;
			if (ImGui::IsKeyPressed(k.key, false))       // false = no auto-repeat
				D2VInput_PostKey(k.vk, 1);
			else if (ImGui::IsKeyReleased(k.key))
				D2VInput_PostKey(k.vk, 0);
		}
	}
}

// Draw the panel. Called once per debugger frame from the standalone loop.
void D2DebugGamePanel()
{
	if (!g_open)
	{
		// Closed, not merely collapsed: stop the game-thread expansion entirely.
		if (D2Capture_StreamEnabled())
			D2Capture_StreamEnable(0);
		return;
	}
	// A first-run size that can actually SHOW a frame, and a floor that keeps it
	// showable. Without the floor the window opened as a ~60px strip -- title bar
	// and controls only, zero content region -- so frames streamed correctly and
	// were drawn into nothing, which reads exactly like a broken capture.
	// The constraint (not just the default) matters because imgui.ini persists a
	// previous bad size and FirstUseEver will not override it.
	ImGuiWindowFlags wflags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	if (g_lockNative && g_texW > 0 && g_texH > 0 && g_chromeTop > 0.0f)
	{
		// Exactly the frame plus the chrome -- no letterbox, no padding slack.
		ImGui::SetNextWindowSize(ImVec2((float)g_texW + g_chromeSide * 2.0f,
		                                (float)g_texH + g_chromeTop +
		                                    ImGui::GetStyle().WindowPadding.y),
		                         ImGuiCond_Always);
		// Locked means locked: no resize grips, so a stray drag cannot knock it
		// off 1:1.
		wflags |= ImGuiWindowFlags_NoResize;

		// GROW THE HOST if it cannot contain the panel. An ImGui window is
		// clipped to its viewport, so with 1:1 on by default and a host window
		// smaller than the frame, the game would simply be cut off -- which
		// looks like a broken lock rather than a too-small window. Only ever
		// grows, so it will not fight a host the operator made larger.
		if (HWND host = D2Panel_GetHostWindow())
		{
			const int needW = (int)((float)g_texW + g_chromeSide * 2.0f) + 24;
			const int needH = (int)((float)g_texH + g_chromeTop +
			                        ImGui::GetStyle().WindowPadding.y) + 48;
			RECT cr{};
			if (GetClientRect(host, &cr) &&
			    (cr.right < needW || cr.bottom < needH))
			{
				RECT wr{};
				GetWindowRect(host, &wr);
				const int frameW = (wr.right - wr.left) - cr.right;
				const int frameH = (wr.bottom - wr.top) - cr.bottom;
				SetWindowPos(host, nullptr, 0, 0,
				             (cr.right < needW ? needW : cr.right) + frameW,
				             (cr.bottom < needH ? needH : cr.bottom) + frameH,
				             SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}
	}
	else
	{
		ImGui::SetNextWindowSize(ImVec2(760.0f, 500.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(360.0f, 280.0f),
		                                    ImVec2(FLT_MAX, FLT_MAX));
	}
	if (!ImGui::Begin("Game", &g_open, wflags))
	{
		// Collapsed: stop paying for the expansion on the game's render thread.
		if (D2Capture_StreamEnabled())
			D2Capture_StreamEnable(0);
		ImGui::End();
		return;
	}

	if (!D2Capture_StreamEnabled())
		D2Capture_StreamEnable(1);

	// Apply the persisted (or default) options once, after ImGui has loaded the
	// ini. Done here rather than at init because the settings are not read
	// until the first frame.
	if (!g_bootApplied)
	{
		g_bootApplied = true;
		if (g_wantHideGame)
			D2GameWindow_SetMode(3);
		D2AudioCap_SetPlayLocal(g_wantAudio ? 1 : 0);
		D2VInput_SetEnabled(g_wantVirtual ? 1 : 0);
	}

	const bool haveFrame = SyncTexture();

	// One row, uniform short labels, detail on hover. Everything goes through
	// Opt() so the options look alike, behave alike and persist alike.
	if (Opt("Virtual", &g_wantVirtual,
	        "Arm virtual input: the game takes its mouse and keys from us\n"
	        "instead of the physical device. EVERYTHING else here depends on\n"
	        "it -- routing does nothing without it, and hiding the OS cursor\n"
	        "is suppressed, because D2's own cursor would not be tracking and\n"
	        "you would be left with no cursor at all."))
		D2VInput_SetEnabled(g_wantVirtual ? 1 : 0);

	Opt("Input", &g_routeInput,
	    "Route mouse and keyboard from this panel into the game.\n"
	    "Needs virtual input enabled.");

	Opt("Cursor", &g_hideCursor,
	    "Hide the Windows arrow over the image so only D2's own cursor shows.\n"
	    "Applies only while input is routed and virtual input is on -- otherwise\n"
	    "the game cursor is not tracking and you would have NO cursor at all.");

	Opt("1:1", &g_lockNative,
	    "Size the window to the frame exactly -- no scaling, no letterboxing.\n"
	    "Follows the game's own resolution, which is not constant: 800x600 at the\n"
	    "menu, 1068x600 in-world, so the window resizes entering or leaving a\n"
	    "game. Resizing is disabled while locked.");

	if (Opt("Hide game", &g_wantHideGame,
	        "Hide the real Diablo II window entirely.\n"
	        "Measured: it keeps presenting at 25 fps while hidden and input still\n"
	        "lands, so this panel is unaffected. Restored when unticked and when\n"
	        "the debugger exits."))
		D2GameWindow_SetMode(g_wantHideGame ? 3 : 0);

	if (Opt("Audio", &g_wantAudio,
	        "Play the captured game audio through D2Debugger.\n"
	        "Captured at the DirectSound buffer level and mixed here, so the same\n"
	        "PCM can be shipped to a remote client. Audible only while a window of\n"
	        "this process has focus."))
		D2AudioCap_SetPlayLocal(g_wantAudio ? 1 : 0);

	// SHIFT-click selects the guarded variant. Read KeyShift BEFORE the
	// checkbox: ImGui's io reflects this frame's state either way, but reading
	// it after leaves the intent depending on widget internals.
	const bool shiftHeld = ImGui::GetIO().KeyShift;
	if (Opt("Capture", &g_lockCursor,
	        "Confine the mouse to the game image so it cannot leave the edges.\n"
	        "SHIFT-CLICK to also keep it out of the bottom HUD strip -- no more\n"
	        "stray clicks opening the belt and drinking a potion mid-fight.\n"
	        "Hold CTRL+ALT to release; click the image to capture again. Those\n"
	        "keys are withheld from the game while held, so releasing never\n"
	        "leaks a keypress into D2."))
	{
		// Shift means "capture, guarding the HUD" -- so it also turns capture ON
		// rather than making you tick twice. A plain click is plain capture.
		if (shiftHeld)
		{
			g_hudGuard = true;
			g_lockCursor = true;
		}
		else
		{
			g_hudGuard = false;
		}
	}

	const bool vin = D2VInput_IsEnabled() != 0;
	// The oracle's /input/mode can flip this behind the panel's back. Follow
	// the truth rather than letting the checkbox drift from reality -- a
	// control that lies about the state it controls is worse than none.
	g_wantVirtual = vin;
	// Report the WINDOW size against what 1:1 requires.
	//
	// NOT GetContentRegionAvail: this runs mid-row, after the checkboxes, so it
	// returns the remainder of the current LINE (535 of a 1084-wide window) --
	// which reads exactly like a broken lock when the lock is fine. Compare the
	// window against frame+chrome instead, which is the thing being asserted.
	{
		const ImVec2 ws = ImGui::GetWindowSize();
		const float wantW = (float)g_texW + g_chromeSide * 2.0f;
		const float wantH = (float)g_texH + g_chromeTop + ImGui::GetStyle().WindowPadding.y;
		const bool exact = g_lockNative && g_texW > 0 &&
		                   (int)ws.x == (int)wantW && (int)ws.y == (int)wantH;
		ImGui::TextDisabled("| %s  frame %dx%d  win %dx%d%s%s  frames:%lu",
		                    vin ? "on" : "OFF", g_texW, g_texH,
		                    (int)ws.x, (int)ws.y,
		                    exact ? "  1:1" : (g_lockNative ? "  (fitting)" : ""),
		                    (g_lockCursor && g_hudGuard) ? "  hud-guard" : "",
		                    g_uploads);
	}
	if (g_routeInput && !vin)
	{
		ImGui::SameLine();
		if (ImGui::SmallButton("enable virtual input"))
			D2VInput_SetEnabled(1);
	}

	if (!haveFrame || !g_tex)
	{
		ImGui::TextUnformatted("waiting for a frame (is the game presenting?)");
		ImGui::End();
		return;
	}

	// Fit the frame to the available region, preserving aspect. Letterboxing
	// beats stretching here: a distorted image would make every routed click
	// land somewhere the operator did not point.
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const float aspect = (float)g_texW / (float)g_texH;
	ImVec2 drawn = avail;
	if (avail.x / (avail.y > 0 ? avail.y : 1.0f) > aspect)
		drawn.x = avail.y * aspect;
	else
		drawn.y = avail.x / aspect;
	if (drawn.x < 1.0f) drawn.x = 1.0f;
	if (drawn.y < 1.0f) drawn.y = 1.0f;

	// The whole content region is an INVISIBLE BUTTON, and the frame is drawn
	// into it by hand.
	//
	// ImGui::Image is not an interactive item, so a drag inside it falls through
	// to the window and MOVES it. In D2 you move by holding the mouse down and
	// dragging in the direction you want to run -- i.e. the single most common
	// action in the game would fling the panel around the screen. Claiming the
	// region as an item makes ImGui treat the press as consumed, so the window
	// is draggable only by its title bar.
	//
	// Both buttons are claimed: right-hold is a skill in D2 and would otherwise
	// drag the window just as readily.
	const ImVec2 regionPos = ImGui::GetCursorScreenPos();
	// Measure the chrome for NEXT frame's size request: distance from the window
	// origin to where the frame actually starts.
	{
		const ImVec2 wp = ImGui::GetWindowPos();
		g_chromeTop = regionPos.y - wp.y;
		g_chromeSide = regionPos.x - wp.x;
	}
	if (g_lockNative && g_texW > 0 && g_texH > 0)
	{
		// 1:1 -- the frame is drawn at its own pixel size, not fitted.
		drawn.x = (float)g_texW;
		drawn.y = (float)g_texH;
	}
	ImGui::InvisibleButton("##game_hit", g_lockNative ? drawn : avail,
	                       ImGuiButtonFlags_MouseButtonLeft |
	                       ImGuiButtonFlags_MouseButtonRight);
	const bool hovered = ImGui::IsItemHovered();

	// Centre the letterboxed frame in the region we just claimed.
	// Locked: the frame IS the region, so no centring offset -- any would be
	// dead space, which is the thing being removed.
	const ImVec2 imgPos = g_lockNative
		? regionPos
		: ImVec2(regionPos.x + (avail.x - drawn.x) * 0.5f,
		         regionPos.y + (avail.y - drawn.y) * 0.5f);
	ImGui::GetWindowDrawList()->AddImage(
		(ImTextureID)g_tex, imgPos,
		ImVec2(imgPos.x + drawn.x, imgPos.y + drawn.y));

	// ---- cursor capture ----------------------------------------------------
	//
	// Clip the REAL pointer to the drawn image. Windows drops a clip whenever
	// the window loses activation, so this is re-applied every frame rather
	// than set once -- and released the moment the window is not focused, or
	// the pointer would stay trapped over another application.
	{
		const bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		if (BreakoutHeld())
			g_brokeOut = true;                 // held: release and stay released
		else if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			g_brokeOut = false;                // clicking back in re-captures

		const bool want = g_lockCursor && focused && !g_brokeOut && g_texW > 0;
		if (want)
		{
			// COORDINATE SPACES. ImGui's "screen position" is relative to its
			// viewport -- the host window's CLIENT AREA -- while ClipCursor takes
			// DESKTOP coordinates. Passing ImGui's values straight through
			// shifted the clip up by the title-bar height, which let the pointer
			// reach the options row above the image and stopped it reaching the
			// bottom edge of the game by the same amount. ClientToScreen is the
			// conversion; without it the rectangle is silently the wrong one.
			HWND host = D2Panel_GetHostWindow();
			POINT tl = { (LONG)imgPos.x, (LONG)imgPos.y };
			// Guarded: stop short of the control panel. Converted from source
			// pixels through the current scale, so it lands on the same place in
			// the game whatever size the window is.
			float bottom = imgPos.y + drawn.y;
			if (g_hudGuard && g_texH > 0)
				bottom -= (float)kHudGuardPx * (drawn.y / (float)g_texH);
			POINT br = { (LONG)(imgPos.x + drawn.x), (LONG)bottom };
			if (host)
			{
				ClientToScreen(host, &tl);
				ClientToScreen(host, &br);
			}
			RECT r{ tl.x, tl.y, br.x, br.y };
			// A degenerate rect would confine the pointer to nothing at all,
			// which reads as a frozen mouse rather than a bad rectangle.
			if (r.right > r.left && r.bottom > r.top)
			{
				// Through the passthrough: our own hook swallows the GAME's clips
				// so it cannot fight the virtual pointer, and would swallow this.
				D2VInput_ClipCursorReal(&r);
			}
			g_captured = true;
		}
		else if (g_captured)
		{
			D2VInput_ClipCursorReal(nullptr);
			g_captured = false;
		}
	}

	if (hovered)
	{
		RouteMouse(imgPos, drawn);

		// Only hide it when the GAME cursor is actually tracking. If routing is
		// off, or virtual input was never armed, D2's cursor sits wherever it
		// last was and hiding the OS one would leave you with NO cursor over the
		// panel -- strictly worse than the two we started with.
		// Re-asserted every frame because ImGui resets the requested cursor each
		// frame; the Win32 backend turns _None into SetCursor(nullptr).
		if (g_hideCursor && g_routeInput && D2VInput_IsEnabled())
			ImGui::SetMouseCursor(ImGuiMouseCursor_None);
	}
	// Keyboard follows FOCUS, not hover. Gating keys on the pointer being over
	// the image means a skill hotkey dies the moment you nudge the mouse off
	// it, which is not how anyone plays.
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
		RouteKeyboard();

	ImGui::End();
}

// Release the device-bound texture WITHOUT tearing the panel down. Must run
// before IDirect3DDevice9::Reset: the texture lives in D3DPOOL_DEFAULT, and a
// surviving default-pool resource makes Reset fail with D3DERR_INVALIDCALL --
// which the host turns straight into IM_ASSERT(0). Any window resize would
// have hit this. The texture is recreated by the next SyncTexture, because
// ReleaseTexture also zeroes the cached size.
// Must run after ImGui::CreateContext and BEFORE the first frame, since the
// ini is parsed on that first NewFrame.
void D2DebugGamePanel_RegisterSettings()
{
	ImGuiSettingsHandler h;
	h.TypeName = "D2Panel";
	h.TypeHash = ImHashStr("D2Panel");
	h.ClearAllFn = SettingsClearAll;
	h.ReadOpenFn = SettingsReadOpen;
	h.ReadLineFn = SettingsReadLine;
	h.WriteAllFn = SettingsWriteAll;
	ImGui::AddSettingsHandler(&h);
}

void D2DebugGamePanel_ReleaseDeviceObjects()
{
	ReleaseTexture();
}

// Called from the render loop's teardown so the texture does not outlive the
// device (a lost device would otherwise leave a dangling D3DPOOL_DEFAULT
// resource, which is exactly what makes a Reset fail).
void D2DebugGamePanel_Shutdown()
{
	// Never leave the pointer trapped in a rectangle belonging to a window
	// that is going away.
	if (g_captured)
	{
		D2VInput_ClipCursorReal(nullptr);
		g_captured = false;
	}
	ReleaseTexture();
	D2Capture_StreamEnable(0);
}
