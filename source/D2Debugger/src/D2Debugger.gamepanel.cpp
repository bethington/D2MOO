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
#include "D2Debugger.canvas.h"
#include <d3d9.h>
#include <windows.h>
#include <algorithm>
#include <cfloat>
#include <climits>

// ---- capture (D2Debugger.vcapture.cpp) --------------------------------------
extern "C" void D2Capture_StreamEnable(int on);
extern "C" int  D2Capture_StreamEnabled();
extern "C" unsigned long D2Capture_StreamSeq();
extern "C" const unsigned char* D2Capture_LockFrame(int* w, int* h, unsigned long* seq);
extern "C" void D2Capture_UnlockFrame();

// ---- virtual input (D2Debugger.vinput.cpp) ----------------------------------
extern "C" void D2VInput_SetEnabled(int on);
extern "C" void D2VInput_SetMode(int mode);   // 0 off, 1 virtual, 2 physical
extern "C" int  D2VInput_Mode();
extern "C" int  D2VInput_IsEnabled();
extern "C" int  D2VInput_MoveToGameXY(int gameX, int gameY, int* outX, int* outY);
extern "C" int  D2VInput_PostMouseMove(int clientX, int clientY);
extern "C" void D2VInput_SetScreenPos(int x, int y);
extern "C" void D2VInput_SetKey(int vk, int down);
extern "C" int  D2VInput_PostMouseButton(int button, int down, int clientX, int clientY);
extern "C" int  D2VInput_PostKey(int vk, int down);
extern "C" int  D2VInput_GetKey(int vk);
extern "C" int  D2VInput_RealKeyDown(int vk);
extern "C" void D2VInput_MarkRenderThread();
extern "C" int  D2VInput_PostAltHold(int down);
extern "C" int  D2VInput_ClipCursorReal(const void* rect);
extern "C" int  D2GameWindow_SetMode(int mode);
extern "C" int  D2GameWindow_Mode();
extern "C" int  D2GameWindow_ApplyFullscreenClip();
extern "C" void D2AudioCap_SetPlayLocal(int on);
extern "C" int  D2AudioCap_PlayLocal();
// Where this panel is currently anchored (D2Debugger.snap.cpp). Reported in the
// status line only -- the snap layer owns the value, the panel just shows it,
// so there is no second copy to drift.
extern "C" const char* D2Snap_AnchorName(const char* window);
// Is the host window currently filling its monitor (D2Debugger.hostwindow.cpp)?
extern "C" int D2Host_IsFilled();

// The debugger's own D3D9 device, owned by D2Debugger.imgui.d3d9.cpp.
LPDIRECT3DDEVICE9 D2Panel_GetDevice();
HWND D2Panel_GetHostWindow();

namespace
{
	LPDIRECT3DTEXTURE9 g_tex = nullptr;
	int g_texW = 0, g_texH = 0;
	unsigned long g_lastSeq = 0;
	bool g_open = true;
	// INPUT, the single switch. Arms the virtual-input layer AND routes this
	// panel's mouse and keys into the game; unticked, nothing is sent at all.
	//
	// There used to be a second option, "Virtual", for arming the layer. The
	// split was indefensible: the Post* functions that actually deliver input
	// never checked the armed flag, so unticking Virtual changed nothing you
	// could observe -- it only silenced the POLLED hooks the panel never needed.
	// Two names, one idea, neither doing what it said. One switch per thing.
	//
	// PERSISTED, like every other option here. The armed flag is runtime state
	// in the vinput layer and resets on each game restart; when that was the
	// unremembered half, the cursor hide (which is gated on it) silently died
	// after every relaunch with its checkbox still showing ticked.
	bool g_routeInput = true;
	// PHYSICAL rather than virtual key state, selected by SHIFT-enabling Input.
	// Virtual is the default and the right one for remote play and for
	// deterministic proving runs; physical exists for sitting at the machine and
	// wanting your own held modifiers to reach the game directly.
	//
	// Only ever set while ENABLING -- a plain tick always means virtual, so the
	// unusual mode can never be entered by accident or inherited silently.
	bool g_physicalInput = false;
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
	bool g_wantAudio = false;
	// THE REAL GAME WINDOW IS HIDDEN, ALWAYS, while this panel is open. There
	// used to be a "Hide game" checkbox for it, and unticking it appeared to do
	// nothing whatsoever: the debugger is always-on-top, so the window came back
	// exactly where it had been -- underneath us. A control whose visible effect
	// is nil is worse than no control.
	//
	// F11 replaces it, and does the thing the checkbox was reached for: shows
	// the game FULL SCREEN, with the debugger standing down from topmost so you
	// can actually see it. Off-screen and layered survive as HTTP-only modes.
	//
	// THERE IS NO BOOL FOR "are we full screen". The window mode IS that answer
	// (D2GameWindow_Mode() == 4), and a local copy would be a second one: driving
	// mode 4 straight from /window/mode left the bool false while the game was
	// plainly full screen, so the next F11 read the wrong state and did nothing
	// visible. Same defect as the "Hide game" checkbox this replaced.
	// Virtual-input mode to put back when leaving full screen. -1 = nothing
	// saved. While full screen you are playing the real window with a real
	// keyboard, and VIRTUAL mode makes the hooks report SYNTHETIC key state --
	// so held Shift/Ctrl/Alt would be invisible to the game, which is the same
	// poisoning that once broke every shift-click option in this panel.
	int g_savedInputMode = -1;
	// PROCESS LOOPBACK rather than our DirectSound mixer, selected by
	// SHIFT-enabling Audio. The point is the game's audio with none of our
	// interference in it: every write-side hook stands down and we tap what
	// the process actually renders.
	//
	// You then hear the GAME, not our reproduction -- never both, since our
	// playback would feed back into the same tap. It stays audible while
	// hidden, because muting it would capture silence. Only ever set while
	// ENABLING, so a plain tick always returns to the mixer.
	// CURSOR CAPTURE. Off by default -- confining the operator's pointer
	// without being asked is hostile, and this is an explicit 'playing now' mode.
	bool g_lockCursor = false;
	// Keep the pointer out of the bottom HUD strip while captured, so a
	// mis-aimed click cannot open the belt and drink a potion mid-fight.
	// Set by RIGHT-SHIFT-clicking Capture.
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
	// Last routed game-space position, for releasing a held button on blur.
	int g_lastGx = 0, g_lastGy = 0;
	// Focus last frame, to catch the focused -> unfocused EDGE exactly once.
	bool g_hadFocus = false;
	// Geometry snapshot for the diagnostic endpoint (see /input/state).
	int g_dbgImgX = 0, g_dbgImgY = 0, g_dbgDrawW = 0, g_dbgDrawH = 0;
	int g_dbgScrL = 0, g_dbgScrT = 0, g_dbgScrR = 0, g_dbgScrB = 0;
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
		// HideGame is gone -- the game is now always hidden while the panel is
		// open, and F11 is what shows it. Read and discarded so ini files
		// written by earlier builds still parse, same as Capture below.
		else if (sscanf_s(line, "HideGame=%d", &v) == 1)   { /* retired */ }
		else if (sscanf_s(line, "Audio=%d", &v) == 1)      g_wantAudio = v != 0;
		// Capture is deliberately NOT restored. It confines the operator's
		// pointer, and restoring it means the very first frame of a new
		// session can trap the mouse before anyone has asked for it --
		// which is exactly what happened, and on a scaled display the
		// rectangle is wrong as well (measured 0.8x the image), so the
		// edges of the game are unreachable and the only way out was to
		// kill the game. Read and discarded so old ini files still parse.
		else if (sscanf_s(line, "Capture=%d", &v) == 1)    { /* not restored */ }
		else if (sscanf_s(line, "HudGuard=%d", &v) == 1)   g_hudGuard = v != 0;
		else if (sscanf_s(line, "Physical=%d", &v) == 1)   g_physicalInput = v != 0;
	}

	void SettingsWriteAll(ImGuiContext*, ImGuiSettingsHandler* h, ImGuiTextBuffer* buf)
	{
		buf->appendf("[%s][Settings]\n", h->TypeName);
		buf->appendf("Input=%d\n",      g_routeInput ? 1 : 0);
		buf->appendf("HideCursor=%d\n", g_hideCursor ? 1 : 0);
		buf->appendf("Lock11=%d\n",     g_lockNative ? 1 : 0);
		buf->appendf("Audio=%d\n",      g_wantAudio ? 1 : 0);
		buf->appendf("Capture=%d\n",    0);   // never persisted on; see above
		buf->appendf("HudGuard=%d\n",   g_hudGuard ? 1 : 0);
		buf->appendf("Physical=%d\n",   g_physicalInput ? 1 : 0);
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

	// The resting state of the real game window while this panel is open.
	// Named rather than spelled 3 at each call site, because "hidden" is a
	// policy here and the number is just how gamewindow.cpp spells it.
	constexpr int kGameResting = 3;

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
		// The extension's only chance to see the frame's pixels is here, while
		// the lock is held. It samples a 64x36 grid rather than copying, and
		// rate-limits itself, so this costs the render thread microseconds --
		// and it costs nothing at all in-world, where the frame already covers
		// the canvas and there is nothing to extend.
		D2Canvas_NoteFrame(w, h);
		D2Canvas_SampleFrame(src, w, h);
		D2Capture_UnlockFrame();

		if (ok)
		{
			g_lastSeq = seq;
			++g_uploads;
		}
		return ok;
	}

	// ImGui units per CURSOR unit.
	//
	// ImGui renders into the physical backbuffer (viewport measured 2560x1440)
	// while this DPI-virtualised process reports a 2048x1152 client and feeds
	// io.MousePos from GetCursorPos in those same logical units. So an ImGui
	// coordinate C is on screen at cursor position C / S.
	//
	// MEASURED from the two spaces themselves, not from GetDpiForWindow: three
	// separate attempts to derive this from a DPI API produced wrong answers,
	// and this ratio is the thing that actually matters -- whatever the reason
	// the two disagree.
	float ImGuiToCursorScale()
	{
		const ImGuiViewport* vp = ImGui::GetMainViewport();
		if (!vp || vp->Size.y <= 1.0f)
			return 1.0f;
		HWND h = D2Panel_GetHostWindow();
		if (!h)
			return 1.0f;
		RECT rc{};
		if (!GetClientRect(h, &rc))
			return 1.0f;
		const float ch = (float)(rc.bottom - rc.top);
		if (ch <= 1.0f)
			return 1.0f;
		const float s = vp->Size.y / ch;
		// Sanity: only believe a plausible display scale. Anything else means
		// one of the two reads is not what it is assumed to be, and 1.0 (i.e.
		// no conversion) is the safe answer.
		if (s < 0.5f || s > 4.0f)
			return 1.0f;
		return s;
	}

	// Map a position inside the drawn image back to GAME pixels and push it
	// through virtual input. `img` is the top-left of the image in screen space.
	void RouteMouse(const ImVec2& img, const ImVec2& drawn)
	{
		if (!g_routeInput || g_texW <= 0 || g_texH <= 0)
			return;
		ImGuiIO& io = ImGui::GetIO();
		// The image in CURSOR units, which is the space io.MousePos is in.
		// Dividing by S converts the ImGui rectangle to where it actually is on
		// screen; without it the game's full width is spread across 1.25x too
		// many cursor pixels and the in-game cursor trails the real one, badly
		// enough that the right and bottom edges are unreachable.
		const float S = ImGuiToCursorScale();
		const float ox = img.x / S;
		const float oy = img.y / S;
		const float ow = (drawn.x > 0 ? drawn.x : 1.0f) / S;
		const float oh = (drawn.y > 0 ? drawn.y : 1.0f) / S;
		const float fx = (io.MousePos.x - ox) / ow;
		const float fy = (io.MousePos.y - oy) / oh;
		if (fx < 0.0f || fx > 1.0f || fy < 0.0f || fy > 1.0f)
			return;                          // outside the image: not ours to route

		const int gx = (int)(fx * (float)g_texW);
		const int gy = (int)(fy * (float)g_texH);

		// POST AND MOVE ON. Emphatically not MoveToGameXY here: that helper
		// blocks for up to 10 x Sleep(20) waiting for D2Client's g_nMouseX/Y to
		// confirm the move. In-world they update every frame and it returns on
		// the first check; AT THE MENU D2Client is not running its game loop, so
		// they never update, the full 200 ms is burned, and it happens on every
		// panel frame -- the render thread drops to ~5 fps and the cursor crawls.
		// The confirming variant is still what /input/move uses, where a
		// synchronous answer is the point.
		//
		// Only on CHANGE. PostMessage does not coalesce WM_MOUSEMOVE the way real
		// hardware input does -- every call queues a discrete message -- so
		// re-posting an unchanged position only gives the menu's pump more work.
		static int s_lastPostedX = INT_MIN, s_lastPostedY = INT_MIN;
		if (gx != s_lastPostedX || gy != s_lastPostedY)
		{
			s_lastPostedX = gx;
			s_lastPostedY = gy;
			if (D2VInput_PostMouseMove(gx, gy))
				D2VInput_SetScreenPos(gx, gy);   // keep the virtual view in step
		}
		// Where a button-up should land if focus is lost while held: the last
		// place the pointer actually was, not (0,0), which D2 would read as a
		// click in the top-left corner of the world.
		g_lastGx = gx;
		g_lastGy = gy;

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

	// Is the cursor break-out combo held? RIGHT Ctrl + RIGHT Alt, specifically.
	//
	// The LEFT Ctrl/Alt are deliberately NOT part of this: the game needs them.
	// Ctrl-click moves an item to the stash/cube/belt, Alt shows items on the
	// ground, and both are common enough that stealing that pair for the panel
	// would make the game unplayable through it. Binding break-out to the RIGHT
	// side keeps a dedicated escape while leaving the LEFT side entirely to the
	// game -- and since only the LEFT modifiers are routed (see kKeys), the
	// RIGHT break-out keys can never leak into D2 either.
	bool BreakoutHeld()
	{
		// The PHYSICAL right Ctrl + right Alt, read past our own hook. Not
		// ImGui: in virtual mode our GetKeyState hook feeds ImGui the synthetic
		// (empty) modifier state, so ImGui's IsKeyDown for the modifiers is
		// unreliable -- the same poisoning that broke gameplay modifier routing.
		// The physical read is correct in BOTH modes.
		return (D2VInput_RealKeyDown(VK_RCONTROL) && D2VInput_RealKeyDown(VK_RMENU));
	}

	// Drop everything the panel is holding. Called on the focus-loss edge.
	//
	// RouteKeyboard and RouteMouse run ONLY while the panel is focused, so the
	// release half of any held input is simply never delivered once focus goes
	// -- ImGui does clear its own key state on WM_KILLFOCUS, but that release
	// arrives on a frame where we are no longer looking. The synthetic key then
	// stays down forever and the game keeps polling it ~87 times a second.
	//
	// Buttons first and they matter most: click-and-hold is how you RUN in D2,
	// so Alt-Tabbing mid-run used to leave the character running with nothing
	// able to stop it.
	void ReleaseAllHeld()
	{
		static const int kBtnVks[] = { VK_LBUTTON, VK_RBUTTON };
		for (int vk : kBtnVks)
			if (D2VInput_GetKey(vk))
				D2VInput_PostMouseButton(vk, 0, g_lastGx, g_lastGy);
		for (const KeyPair& k : kKeys)
			if (D2VInput_GetKey(k.vk))
				D2VInput_PostKey(k.vk, 0);
	}

	void RouteKeyboard()
	{
		if (!g_routeInput)
			return;
		// TRANSITIONS, posted as real messages. Re-asserting the polled state
		// every frame was never enough on its own -- exactly the failure the
		// mouse buttons had, where the state was right and nothing happened.
		for (const KeyPair& k : kKeys)
		{
			// The three modifiers are handled separately below: ImGui CANNOT
			// see them in virtual mode. Its Win32 backend disambiguates
			// VK_SHIFT/CONTROL/MENU into left/right via GetKeyState, which our
			// own hook answers with the synthetic (empty) state -- so ImGui
			// never registers the modifier and IsKeyPressed never fires here.
			if (k.vk == VK_SHIFT || k.vk == VK_CONTROL || k.vk == VK_MENU)
				continue;
			if (ImGui::IsKeyPressed(k.key, false))       // false = no auto-repeat
				D2VInput_PostKey(k.vk, 1);
			else if (ImGui::IsKeyReleased(k.key))
				D2VInput_PostKey(k.vk, 0);
		}

		// MODIFIERS, from the PHYSICAL device past our own hook. ImGui's view is
		// poisoned (above), so read the real keyboard and mirror it into the
		// synthetic polled state every frame -- level-triggered, so it is robust
		// against focus flaps, and it is exactly what D2 reads
		// (GetAsyncKeyState(VK_SHIFT), measured 75/sec in-world).
		//
		// Shift is the generic (either shift) -- it is not part of break-out and
		// players buy stacks with whichever they have. Ctrl/Alt are LEFT only:
		// the RIGHT pair is the cursor break-out, so reading the left keeps that
		// combo from ever leaking a modifier into the game.
		// Shift + Ctrl are consumed from the POLLED state / click MK_ flags, so
		// mirroring g_down is enough for them (measured: Shift is polled via
		// GetAsyncKeyState, Ctrl arrives on the click's MK_CONTROL).
		struct Mod { int phys; int game; };
		static const Mod kMods[] = {
			// LEFT shift only. The RIGHT shift is the panel's own option
			// modifier (shift-click a checkbox), so sending it to the game as
			// well would fire both at once -- picking an option would also
			// shift-click in D2.
			{ VK_LSHIFT,   VK_SHIFT },     // left shift only
			{ VK_LCONTROL, VK_CONTROL },   // left ctrl only
		};
		for (const Mod& m : kMods)
			D2VInput_SetKey(m.game, D2VInput_RealKeyDown(m.phys) ? 1 : 0);

		// ALT is different: the game never polls it, it tracks it from the
		// WM_SYSKEYDOWN/UP message. Drive that from the LEFT Alt transition
		// (the RIGHT Alt is reserved for the cursor break-out). Edge-triggered,
		// re-synced each frame so a focus flap cannot leave it stuck.
		static bool s_altDown = false;
		const bool altNow = D2VInput_RealKeyDown(VK_LMENU) != 0;
		if (altNow != s_altDown)
		{
			s_altDown = altNow;
			D2VInput_PostAltHold(altNow ? 1 : 0);
		}
	}
}

// Draw the panel. Called once per debugger frame from the standalone loop.
// Enter (on != 0) or leave the game's full-screen mode. The single entry point
// for it: F11 from the debugger's window proc, F11 from the game's own proc
// once it has focus, and teardown all come through here, so the input parking
// below cannot be skipped by one of those routes.
//
// The Z-order half is gamewindow.cpp's, deliberately: it already owns "what
// state is the game window in", and splitting that across two files is how the
// two would drift.
extern "C" void D2GamePanel_SetGameFullscreen(int on)
{
	if (on)
	{
		// Park virtual input BEFORE the window moves, so no synthetic state is
		// left applied over a window you are about to type into for real. Only
		// captured once: entering twice (F11 after an HTTP-driven mode 4) must
		// not overwrite the real saved mode with the 0 we just set.
		if (g_savedInputMode < 0)
			g_savedInputMode = D2VInput_Mode();
		D2VInput_SetMode(0);
		if (!D2GameWindow_SetMode(4))
		{
			// No game window: put the input layer back rather than leaving it
			// silently off, which would look exactly like the Input checkbox
			// having stopped working.
			D2VInput_SetMode(g_savedInputMode);
			g_savedInputMode = -1;
		}
	}
	else
	{
		D2GameWindow_SetMode(kGameResting);
		if (g_savedInputMode >= 0)
			D2VInput_SetMode(g_savedInputMode);
		g_savedInputMode = -1;
	}
}

// Asked of the window layer, never of a local flag -- see the note by
// g_savedInputMode. This is what makes F11 agree with /window/mode.
extern "C" int D2GamePanel_IsGameFullscreen()
{
	return D2GameWindow_Mode() == 4 ? 1 : 0;
}

// Did the Game panel have focus as of the last completed frame? Read by the
// F11 router in D2Debugger.imgui.d3d9.cpp, which runs during the message pump
// and so cannot ask ImGui directly. g_hadFocus is already maintained for the
// key-release-on-blur path -- reusing it keeps one answer to one question.
extern "C" int D2Panel_GameFocused() { return g_hadFocus ? 1 : 0; }

void D2DebugGamePanel()
{
	// Tag this as the render thread so the key-poll histogram can exclude
	// ImGui's own backend polls and show only what the game reads.
	D2VInput_MarkRenderThread();
	if (!g_open)
	{
		// Closed, not merely collapsed: stop the game-thread expansion entirely.
		if (D2Capture_StreamEnabled())
			D2Capture_StreamEnable(0);
		// AND give the real window back. Hiding it is only defensible while
		// this panel is the thing you are looking at instead; with the panel
		// closed and the checkbox retired, a still-hidden game would leave no
		// visible way to reach it at all.
		if (D2GamePanel_IsGameFullscreen())
			D2GamePanel_SetGameFullscreen(0);
		if (D2GameWindow_Mode() != 0)
			D2GameWindow_SetMode(0);
		g_bootApplied = false;        // re-hide if the panel is reopened
		return;
	}
	// A first-run size that can actually SHOW a frame, and a floor that keeps it
	// showable. Without the floor the window opened as a ~60px strip -- title bar
	// and controls only, zero content region -- so frames streamed correctly and
	// were drawn into nothing, which reads exactly like a broken capture.
	// The constraint (not just the default) matters because imgui.ini persists a
	// previous bad size and FirstUseEver will not override it.
	ImGuiWindowFlags wflags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	// THE CANVAS, not the live frame. D2 renders the menu at 800x600 and the
	// splash at 640x480 while the world runs at whatever ddraw.ini declares, so
	// sizing this window from the frame made it resize under you every time you
	// entered or left a game. The canvas is the configured resolution and does
	// not change, so the window finally holds still; the frame is drawn into it
	// at 1:1 and centred, with D2Canvas filling whatever is left over.
	int canvasW = 0, canvasH = 0;
	D2Canvas_Size(&canvasW, &canvasH);
	if (g_lockNative && canvasW > 0 && canvasH > 0 && g_chromeTop > 0.0f)
	{
		// Exactly the canvas plus the chrome -- no letterbox, no padding slack.
		ImGui::SetNextWindowSize(ImVec2((float)canvasW + g_chromeSide * 2.0f,
		                                (float)canvasH + g_chromeTop +
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
		//
		// NOT WHILE THE HOST FILLS ITS MONITOR. It cannot need growing then, and
		// this arithmetic is what turned an already-shrunken host into a 1112px
		// one -- panel width exactly -- which, with the fill's title bar and
		// grips gone, is a window you cannot get back by hand. If a filled host
		// is somehow too small, D2Host_Tick puts it back to the MONITOR size,
		// which is the right answer rather than this one.
		if (HWND host = D2Host_IsFilled() ? nullptr : D2Panel_GetHostWindow())
		{
			const int needW = (int)((float)canvasW + g_chromeSide * 2.0f) + 24;
			const int needH = (int)((float)canvasH + g_chromeTop +
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

	// NO on-screen clamp here -- it was tried and REVERTED.
	//
	// It compared ImGui coordinates against GetClientRect and they are in
	// DIFFERENT SPACES: ImGui's viewport is 2560x1440 (physical) while
	// GetClientRect returns 2048x1152 (logical) in this DPI-virtualised
	// process. The snap layer is right -- it anchors against vp->Size, which is
	// why the Game panel sits at 771+669=1440, exactly the viewport bottom.
	// Clamping against 1152 just hauled the window 288px up for no reason.
	//
	// That 1440-vs-1152 split is the real defect behind the cursor clip: the
	// rect is built from ImGui (physical) values and handed to ClipCursor, which
	// works in the process's logical cursor space. Fix that conversion, not the
	// window position.

	// Apply the persisted (or default) options once, after ImGui has loaded the
	// ini. Done here rather than at init because the settings are not read
	// until the first frame.
	if (!g_bootApplied)
	{
		g_bootApplied = true;
		// Unconditional now: the panel being open IS the statement that you are
		// watching the game here rather than in its own window. F11 is the way
		// to see the real one.
		D2GameWindow_SetMode(kGameResting);
		D2AudioCap_SetPlayLocal(g_wantAudio ? 1 : 0);
		D2VInput_SetMode(g_routeInput ? (g_physicalInput ? 2 : 1) : 0);
	}

	// Hold the pointer on the full-screen game, re-asserted every frame because
	// Windows drops a clip on focus changes, display changes and UAC prompts --
	// the same reason the panel's own Capture re-applies below.
	//
	// CTRL+ALT releases it while held, the identical idiom Capture uses. There
	// is always a way to get the mouse back that is not "kill the game", which
	// this codebase has had to do before.
	if (D2GamePanel_IsGameFullscreen())
	{
		const bool letGo = D2VInput_RealKeyDown(VK_CONTROL) && D2VInput_RealKeyDown(VK_MENU);
		if (letGo)
			D2VInput_ClipCursorReal(nullptr);
		else
			D2GameWindow_ApplyFullscreenClip();
	}

	const bool haveFrame = SyncTexture();

	// One row, uniform short labels, detail on hover. Everything goes through
	// Opt() so the options look alike, behave alike and persist alike.
	// SHIFT-enable selects physical state. Read KeyShift BEFORE the checkbox:
	// io reflects this frame's state either way, but reading it after leaves the
	// intent depending on widget internals. Shared with Capture below.
	// The panel's option modifier: the PHYSICAL right Shift, read past our own
	// hook.
	//
	// NOT ImGui::GetIO().KeyShift -- in virtual mode our GetKeyState hook feeds
	// ImGui the synthetic modifier state, so io.KeyShift reads false however
	// hard the operator holds the key, and every shift-click option silently
	// stopped working. Same poisoning that broke the gameplay modifiers.
	//
	// RIGHT specifically: the LEFT Shift belongs to the game now (buy a stack,
	// attack in place), so using it here would do both at once.
	const bool shiftHeld = D2VInput_RealKeyDown(VK_RSHIFT) != 0;
	if (Opt("Input", &g_routeInput,
	        "Route this panel's mouse and keyboard into the game.\n"
	        "Unticked, NOTHING is sent and the game falls back to your physical\n"
	        "mouse and keyboard.\n"
	        "\n"
	        "Always VIRTUAL: the game's key state is exactly what we send, so a\n"
	        "remote client works and proving runs stay reproducible.\n"
	        "RIGHT-SHIFT-CLICK to enable in PHYSICAL state instead -- D2 then reads the\n"
	        "real keyboard for held modifiers (Shift/Ctrl/Alt), which works while\n"
	        "you are at the machine but leaks: those reads ignore focus, so\n"
	        "holding Shift in another app reaches the game too. Plain-ticking\n"
	        "always returns to virtual.\n"
	        "\n"
	        "Hiding the OS cursor depends on this -- with it off, D2's own cursor\n"
	        "is not tracking, so hiding yours would leave you with none at all."))
	{
		// Only on the way ON, so a plain tick is always the safe mode and
		// unticking never silently rearms physical next time.
		if (g_routeInput)
			g_physicalInput = shiftHeld;
		D2VInput_SetMode(g_routeInput ? (g_physicalInput ? 2 : 1) : 0);
	}

	Opt("Cursor", &g_hideCursor,
	    "Hide the Windows arrow over the image so only D2's own cursor shows.\n"
	    "Applies only while Input is on -- otherwise the game cursor is not\n"
	    "tracking and you would have NO cursor at all.");

	Opt("1:1", &g_lockNative,
	    "Size the window to the resolution the game is SET to, and draw the frame\n"
	    "inside it at exactly one screen pixel per game pixel -- never scaled.\n"
	    "\n"
	    "The size comes from ddraw.ini (and grows if a bigger frame ever shows up),\n"
	    "NOT from the current frame -- D2 renders the menu at 800x600 and the splash\n"
	    "at 640x480 while the world runs at 1068x600, so following the frame meant\n"
	    "the window resized under you every time you entered or left a game.\n"
	    "\n"
	    "A frame smaller than that is centred, and the space around it is filled\n"
	    "with baked art if any is present in the extension folder, otherwise with\n"
	    "the frame's own edges mirrored and blurred. Resizing is disabled while\n"
	    "locked; untick for a free-resize window with the frame fitted to it.");

	// No "Hide game" checkbox any more. The real window is hidden for as long as
	// this panel is open, and F11 -- with this panel focused -- shows it FULL
	// SCREEN instead. The old checkbox could only ever put the window back where
	// it was, underneath an always-on-top debugger, so unticking it looked like
	// it did nothing at all.

	if (Opt("Audio", &g_wantAudio,
	        "Play the captured game audio through D2Debugger.\n"
	        "Read straight out of dsound-headless -- the DLL the game plays\n"
	        "through -- and mixed here, so the same PCM can be shipped to a\n"
	        "remote client. Audible only while a window of this process has\n"
	        "focus; the remote stream keeps running regardless.\n"
	        "\n"
	        "There is no loopback alternative any more: it tapped what the\n"
	        "process renders natively, and the shim renders to no device."))
	{
		D2AudioCap_SetPlayLocal(g_wantAudio ? 1 : 0);
	}

	// RIGHT-SHIFT-click selects the guarded variant, same idiom as Input above and
	// reusing the shiftHeld read from there.
	if (Opt("Capture", &g_lockCursor,
	        "Confine the mouse to the game image so it cannot leave the edges.\n"
	        "RIGHT-SHIFT-CLICK to also keep it out of the bottom HUD strip -- no more\n"
	        "stray clicks opening the belt and drinking a potion mid-fight.\n"
	        "Hold CTRL+ALT to release; click the image to capture again. Those\n"
	        "keys are withheld from the game while held, so releasing never\n"
	        "leaks a keypress into D2."))
	{
		// Asking for capture CLEARS the break-out latch. g_brokeOut is set by
		// Ctrl+Alt and was otherwise only cleared by clicking the image, so a
		// stale latch -- now easy to set, since the break-out reads the physical
		// keys and fires for Ctrl+Alt pressed in ANY application -- silently
		// beat the checkbox: ticking Capture did nothing whatsoever and gave no
		// hint why. An explicit tick is an unambiguous statement of intent and
		// must not lose to a keypress from minutes ago.
		if (g_lockCursor)
			g_brokeOut = false;

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

	// Reported in the status line only. Input is the source of truth now, so do
	// NOT mirror this back into it -- the oracle's /input/mode can flip the layer
	// behind the panel's back, and that must not silently retick the operator's
	// own control.
	const int imode = D2VInput_Mode();
	const char* modeStr = imode == 0 ? "OFF" : imode == 1 ? "virtual" : "PHYSICAL";
	// The REAL game window's state, asked of the layer that owns it. There is no
	// checkbox mirroring it any more, and the oracle's /window/mode can still
	// change it from under us, so this is the only honest source.
	const int wmode = D2GameWindow_Mode();
	const char* winStr = wmode == 0 ? "shown"
	                   : wmode == 1 ? "offscreen"
	                   : wmode == 2 ? "alpha0"
	                   : wmode == 4 ? "FULLSCREEN"
	                                : "hidden";
	// Report the WINDOW size against what 1:1 requires.
	//
	// NOT GetContentRegionAvail: this runs mid-row, after the checkboxes, so it
	// returns the remainder of the current LINE (535 of a 1084-wide window) --
	// which reads exactly like a broken lock when the lock is fine. Compare the
	// window against frame+chrome instead, which is the thing being asserted.
	{
		const ImVec2 ws = ImGui::GetWindowSize();
		int cw = 0, ch = 0;
		D2Canvas_Size(&cw, &ch);
		const float wantW = (float)cw + g_chromeSide * 2.0f;
		const float wantH = (float)ch + g_chromeTop + ImGui::GetStyle().WindowPadding.y;
		const bool exact = g_lockNative && cw > 0 &&
		                   (int)ws.x == (int)wantW && (int)ws.y == (int)wantH;
		const char* snap = D2Snap_AnchorName("Game");
		// The canvas and the frame are reported SEPARATELY. They differ at the
		// menu by design now, and a single number could not tell you whether an
		// 800-wide frame meant "the menu, correctly centred" or "the lock is
		// broken" -- which is the same confusion the win-vs-want pair fixed.
		ImGui::TextDisabled(
			"| %s  game:%s  canvas %dx%d(%s)  frame %dx%d%s  win %dx%d%s%s  snap:%s  frames:%lu",
			modeStr, winStr, cw, ch, D2Canvas_SourceName(), g_texW, g_texH,
			(g_lockNative && (g_texW < cw || g_texH < ch))
				? (strcmp(D2Canvas_FillName(), "baked") == 0 ? " +baked" : " +fill")
				: "",
			(int)ws.x, (int)ws.y,
			exact ? "  1:1" : (g_lockNative ? "  (fitting)" : ""),
			(g_lockCursor && g_hudGuard) ? "  hud-guard" : "",
			snap, g_uploads);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(
			    "Drag the title bar near a corner, an edge centre or the centre of\n"
			    "the debugger window and the panel snaps onto it; the anchor is\n"
			    "remembered in imgui.ini and re-applied every frame, so it holds\n"
			    "when the game's resolution changes (800x600 at the menu,\n"
			    "1068x600 in-world) and when the host window is resized.\n"
			    "\n"
			    "NUMPAD 1-9 place the focused panel on the matching target --\n"
			    "the keypad layout IS the screen layout. NUMPAD 0 releases it.\n"
			    "\n"
			    "F11 with THIS panel focused shows the real game FULL SCREEN\n"
			    "(borderless, filling the monitor); the debugger stands down\n"
			    "from always-on-top so you can see it, and virtual input is\n"
			    "suspended so your real keyboard reaches the game. F11 again\n"
			    "brings it back -- from either window, so a full-screen game is\n"
			    "never something you have to kill to escape.\n"
			    "With any OTHER panel focused, F11 keeps its old meaning:\n"
			    "borderless-fill for the debugger's own window.");
	}
	// No inline "enable virtual input" button here on purpose, and no separate
	// Virtual checkbox either -- both were second controls for the state Input
	// already owns. One switch per thing.

	// Keyboard follows FOCUS, not hover. Gating keys on the pointer being over
	// the image means a skill hotkey dies the moment you nudge the mouse off it,
	// which is not how anyone plays.
	//
	// ABOVE the no-frame return on purpose, and it needs no frame to work: a
	// blur during a presentation stall would otherwise skip the release and
	// leave g_hadFocus stale, stranding whatever was held at exactly the moment
	// things were already going wrong.
	const bool hasFocus = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	if (hasFocus)
		RouteKeyboard();
	else if (g_hadFocus)
		ReleaseAllHeld();       // the focused -> unfocused edge, handled once
	g_hadFocus = hasFocus;

	if (!haveFrame || !g_tex)
	{
		// NEVER STRAND THE POINTER. The cursor-capture block is below this
		// return, so from here nothing re-applies the clip AND nothing releases
		// it -- the last one set just stays. Worse, the Ctrl+Alt break-out is
		// evaluated down there too, so the operator has no way out either: the
		// pointer is locked to a window that has stopped drawing, and the escape
		// hatch is unreachable. Release unconditionally before bailing.
		//
		// Reachable any time presenting stops with capture on: the game exiting,
		// a stall, or -- how this was found -- the GDI hooks not being attached
		// yet, which left the panel frameless for 30 seconds with a live clip.
		if (g_captured)
		{
			D2VInput_ClipCursorReal(nullptr);
			g_captured = false;
		}
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
		// 1:1 -- the frame is drawn at its own pixel size, never fitted and
		// never scaled, whatever size the canvas around it is.
		drawn.x = (float)g_texW;
		drawn.y = (float)g_texH;
	}
	// The hit region is the CANVAS when locked, not the frame: it is the whole
	// fixed surface, so the window's draggable area does not shrink at the menu.
	const ImVec2 canvas((float)canvasW, (float)canvasH);
	ImGui::InvisibleButton("##game_hit", g_lockNative ? canvas : avail,
	                       ImGuiButtonFlags_MouseButtonLeft |
	                       ImGuiButtonFlags_MouseButtonRight);
	const bool hovered = ImGui::IsItemHovered();

	// Centre the frame in the region we just claimed. Locked, that region is
	// the canvas, so an 800x600 menu sits centred in 1068x600 with 134px either
	// side -- which is what D2Canvas_DrawBackdrop fills, below.
	const ImVec2 imgPos = g_lockNative
		? ImVec2(regionPos.x + (canvas.x - drawn.x) * 0.5f,
		         regionPos.y + (canvas.y - drawn.y) * 0.5f)
		: ImVec2(regionPos.x + (avail.x - drawn.x) * 0.5f,
		         regionPos.y + (avail.y - drawn.y) * 0.5f);
	// The extension goes down FIRST, under the frame: baked art if any has been
	// authored for this canvas, otherwise the frame's own edges mirrored and
	// blurred. Only draws when the frame is smaller than the canvas, so in-world
	// this is a compare and a return.
	if (g_lockNative)
		D2Canvas_DrawBackdrop(ImGui::GetWindowDrawList(), regionPos, g_texW, g_texH);
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

		// DIAGNOSTIC SNAPSHOT, taken every frame whether or not a clip is
		// applied. Recording it only inside the apply branch made it impossible
		// to inspect the geometry with Capture off -- which is the state the
		// panel now starts in, so the numbers were unreachable exactly when
		// they were needed.
		{
			POINT dtl = { (LONG)imgPos.x, (LONG)imgPos.y };
			POINT dbr = { (LONG)(imgPos.x + drawn.x), (LONG)(imgPos.y + drawn.y) };
			if (HWND dh = D2Panel_GetHostWindow())
			{
				ClientToScreen(dh, &dtl);
				ClientToScreen(dh, &dbr);
			}
			g_dbgImgX = (int)imgPos.x; g_dbgImgY = (int)imgPos.y;
			g_dbgDrawW = (int)drawn.x; g_dbgDrawH = (int)drawn.y;
			g_dbgScrL = dtl.x; g_dbgScrT = dtl.y; g_dbgScrR = dbr.x; g_dbgScrB = dbr.y;
		}

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
				// DPI. ClientToScreen yields coordinates in the WINDOW's logical
				// space; ClipCursor takes PHYSICAL pixels. On a 125% display
				// those differ by exactly 1.25, so the clip came out at 0.8x the
				// drawn image -- measured 1068x600 physical against an image
				// occupying 1335x750, i.e. 267px short on the right and 150px at
				// the bottom, and no way to reach the edges of the game.
				//
				// NO DPI SCALING HERE -- it was tried and REVERTED.
				//
				// Multiplying this rect by dpi/96 made things strictly worse:
				// the clip moved down and right, so the TOP of the game became
				// unreachable where before at least the top-left was. Had the
				// rect simply been logical-where-physical-was-wanted, that
				// scaling would have landed it exactly on the image. It did not,
				// so that model is wrong and nothing here should be built on it.
				//
				// KNOWN REMAINING BUG: on a scaled display the clip still comes
				// out short at the bottom and on the right. /input/geometry
				// reports every input this rect is derived from, and the next
				// attempt should start from a reading rather than a theory --
				// three have now been refuted in a row.
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
		// Armed, not virtual: D2 takes its cursor position from the messages we
		// post in BOTH modes, so its own cursor tracks either way and hiding the
		// OS arrow is right either way.
		if (g_hideCursor && g_routeInput && D2VInput_Mode() != 0)
			ImGui::SetMouseCursor(ImGuiMouseCursor_None);
	}
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

	// The canvas has to know the configured resolution before the first frame
	// request, or the window would be sized once from a default and again a
	// frame later -- the exact resize this whole change exists to remove.
	D2Canvas_Init();
}

void D2DebugGamePanel_ReleaseDeviceObjects()
{
	ReleaseTexture();
	// The extension textures are D3DPOOL_DEFAULT too, and a surviving one is
	// just as capable of failing the device Reset as the frame texture is.
	D2Canvas_ReleaseDeviceObjects();
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

// Geometry the cursor clip is built from, plus the host window's own view of
// itself. Everything needed to see which coordinate space each value is in.
extern "C" void D2Panel_ClipGeometry(int* imgX, int* imgY, int* drawW, int* drawH,
                                     int* scrL, int* scrT, int* scrR, int* scrB,
                                     int* texW, int* texH,
                                     int* cliW, int* cliH,
                                     int* winL, int* winT, int* winR, int* winB,
                                     int* dpi)
{
	if (imgX) *imgX = g_dbgImgX;
	if (imgY) *imgY = g_dbgImgY;
	if (drawW) *drawW = g_dbgDrawW;
	if (drawH) *drawH = g_dbgDrawH;
	if (scrL) *scrL = g_dbgScrL;
	if (scrT) *scrT = g_dbgScrT;
	if (scrR) *scrR = g_dbgScrR;
	if (scrB) *scrB = g_dbgScrB;
	if (texW) *texW = g_texW;
	if (texH) *texH = g_texH;

	RECT cr{ 0, 0, 0, 0 }, wr{ 0, 0, 0, 0 };
	HWND h = D2Panel_GetHostWindow();
	if (h)
	{
		GetClientRect(h, &cr);
		GetWindowRect(h, &wr);
	}
	if (cliW) *cliW = cr.right - cr.left;
	if (cliH) *cliH = cr.bottom - cr.top;
	if (winL) *winL = wr.left;
	if (winT) *winT = wr.top;
	if (winR) *winR = wr.right;
	if (winB) *winB = wr.bottom;

	int d = 96;
	if (h)
	{
		using Fn = UINT(WINAPI*)(HWND);
		static Fn fn = nullptr;
		static bool resolved = false;
		if (!resolved)
		{
			resolved = true;
			if (HMODULE u32 = GetModuleHandleW(L"user32.dll"))
				fn = (Fn)GetProcAddress(u32, "GetDpiForWindow");
		}
		if (fn) d = (int)fn(h);
	}
	if (dpi) *dpi = d;
}
