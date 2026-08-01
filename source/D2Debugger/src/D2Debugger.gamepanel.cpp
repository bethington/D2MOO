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
extern "C" int  D2GameWindow_SetMode(int mode);
extern "C" int  D2GameWindow_Mode();
extern "C" void D2AudioCap_SetPlayLocal(int on);
extern "C" int  D2AudioCap_PlayLocal();

// The debugger's own D3D9 device, owned by D2Debugger.imgui.d3d9.cpp.
LPDIRECT3DDEVICE9 D2Panel_GetDevice();

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
	// Frames the texture actually took, so a stalled panel is visible as a
	// stalled number rather than a still image you might read as a paused game.
	unsigned long g_uploads = 0;

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

	void RouteKeyboard()
	{
		if (!g_routeInput)
			return;
		// TRANSITIONS, posted as real messages. Re-asserting the polled state
		// every frame was never enough on its own -- exactly the failure the
		// mouse buttons had, where the state was right and nothing happened.
		for (const KeyPair& k : kKeys)
		{
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
	ImGui::SetNextWindowSize(ImVec2(760.0f, 500.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(360.0f, 280.0f),
	                                    ImVec2(FLT_MAX, FLT_MAX));
	if (!ImGui::Begin("Game", &g_open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		// Collapsed: stop paying for the expansion on the game's render thread.
		if (D2Capture_StreamEnabled())
			D2Capture_StreamEnable(0);
		ImGui::End();
		return;
	}

	if (!D2Capture_StreamEnabled())
		D2Capture_StreamEnable(1);

	const bool haveFrame = SyncTexture();

	ImGui::Checkbox("Route input", &g_routeInput);
	ImGui::SameLine();
	ImGui::Checkbox("Hide OS cursor", &g_hideCursor);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Hide the Windows arrow over the image so only D2's own "
		                  "cursor shows. Applies only while input is being routed "
		                  "and virtual input is on -- otherwise the game cursor is "
		                  "not tracking and you would have no cursor at all.");
	ImGui::SameLine();
	// Default OFF. The panel is fed by the game presenting into its own window,
	// so anything that stops it drawing blanks this view -- the operator gets to
	// opt in, and toggling off always restores the window.
	bool hidden = D2GameWindow_Mode() != 0;
	if (ImGui::Checkbox("Hide game window", &hidden))
		D2GameWindow_SetMode(hidden ? 3 : 0);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Hide the real Diablo II window entirely. MEASURED: it keeps "
		                  "presenting at 25 fps while hidden and input still lands, so "
		                  "this panel is unaffected. Restored when unticked and when the "
		                  "debugger exits.");
	ImGui::SameLine();
	bool audio = D2AudioCap_PlayLocal() != 0;
	if (ImGui::Checkbox("Audio", &audio))
		D2AudioCap_SetPlayLocal(audio ? 1 : 0);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Play the captured game audio through D2Debugger. Captured at "
		                  "the DirectSound buffer level and mixed here, so the same PCM "
		                  "can be shipped to a remote client. Audible only while a window "
		                  "of this process has focus.");
	ImGui::SameLine();
	const bool vin = D2VInput_IsEnabled() != 0;
	ImGui::TextDisabled("| virtual:%s  %dx%d  frames:%lu",
	                    vin ? "on" : "OFF", g_texW, g_texH, g_uploads);
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
	ImGui::InvisibleButton("##game_hit", avail,
	                       ImGuiButtonFlags_MouseButtonLeft |
	                       ImGuiButtonFlags_MouseButtonRight);
	const bool hovered = ImGui::IsItemHovered();

	// Centre the letterboxed frame in the region we just claimed.
	const ImVec2 imgPos(regionPos.x + (avail.x - drawn.x) * 0.5f,
	                    regionPos.y + (avail.y - drawn.y) * 0.5f);
	ImGui::GetWindowDrawList()->AddImage(
		(ImTextureID)g_tex, imgPos,
		ImVec2(imgPos.x + drawn.x, imgPos.y + drawn.y));

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
void D2DebugGamePanel_ReleaseDeviceObjects()
{
	ReleaseTexture();
}

// Called from the render loop's teardown so the texture does not outlive the
// device (a lost device would otherwise leave a dangling D3DPOOL_DEFAULT
// resource, which is exactly what makes a Reset fail).
void D2DebugGamePanel_Shutdown()
{
	ReleaseTexture();
	D2Capture_StreamEnable(0);
}
