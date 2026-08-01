// D2Debugger.snap.cpp -- magnetic 9-point snapping for every debugger panel,
// remembered across restarts.
//
// THE NINE TARGETS, laid out like a numeric keypad (which is also how the
// hotkeys are bound, so the shortcut needs no memorising):
//
//     7 top-left      8 top-centre      9 top-right
//     4 left-centre   5 centre          6 right-centre
//     1 bottom-left   2 bottom-centre   3 bottom-right      0 = release
//
// Every target is resolved against the HOST WINDOW'S CLIENT AREA, so "centre"
// is the centre of the debugger's own surface and stays correct when the host
// is resized or moved to another monitor.
//
// WHY AN ANCHOR AND NOT A POSITION. imgui.ini already persists each window's
// exact pixel Pos, and that is precisely what does not survive here: the game
// panel is size-locked to the frame it is showing, and the game's render
// resolution is NOT constant -- measured 800x600 at the menu and 1068x600
// in-world. A remembered position drifts off the edge the moment the panel
// resizes under it. Remembering the ANCHOR and re-deriving the position every
// frame is the only version of "remember where it was snapped" that is still
// true after entering a game.
//
// MAGNETISM, and the one non-obvious thing about implementing it. ImGui moves a
// window in UpdateMouseMovingWindowNewFrame (inside NewFrame) by computing an
// ABSOLUTE position from the mouse and the click offset -- not by accumulating
// deltas. That is what makes overriding it safe: we run straight after
// NewFrame, and setting the position ourselves neither accumulates error nor
// desynchronises the drag. Pull the mouse further than the threshold and the
// window snaps straight back to the pointer, because ImGui recomputes it from
// scratch every frame regardless of what we did last frame.
//
// APPLIED BEFORE THE PANELS ARE DRAWN, not after. A window's Begin() uses
// whatever Pos it currently has, so positioning it after its own Begin/End
// would show the old position for one frame -- a visible one-frame lag on every
// re-pin, and re-pins happen on every resolution change. Running first costs
// nothing: we use last frame's Size, which is stable to the pixel except on the
// single frame a window changes size, and that frame is corrected on the next.
//
// PERSISTED in imgui.ini:
//
//     [D2PanelSnap][Settings]
//     Enabled=1
//     Threshold=24
//
//     [D2PanelSnap][Anchors]
//     Game=bottom-center
//     Live Dispatch Registry=top-left
//
// Anchor names are parsed from the RIGHT of the '=' so a window whose title
// contains an '=' still round-trips.

#include "imgui.h"
#include "imgui_internal.h"
#include "D2Debugger.theme.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

extern "C" float D2Host_DpiScale();

namespace
{
	enum Anchor
	{
		Anchor_Free = 0,
		Anchor_TopLeft,
		Anchor_TopCenter,
		Anchor_TopRight,
		Anchor_LeftCenter,
		Anchor_Center,
		Anchor_RightCenter,
		Anchor_BottomLeft,
		Anchor_BottomCenter,
		Anchor_BottomRight,
		Anchor_COUNT
	};

	// Canonical spelling matches fun-doc's config.game_window vocabulary
	// ("bottom-center"), so the two layout systems name the same idea the same
	// way. The British spelling is accepted on read because the comments in this
	// codebase use it and a hand-edited ini should not silently do nothing.
	const char* const kAnchorNames[Anchor_COUNT] = {
		"free",
		"top-left",    "top-center",    "top-right",
		"left-center", "center",        "right-center",
		"bottom-left", "bottom-center", "bottom-right",
	};

	Anchor AnchorFromName(const char* s)
	{
		if (!s)
			return Anchor_Free;
		char norm[64];
		size_t n = 0;
		for (const char* p = s; *p && n + 1 < sizeof(norm); ++p)
		{
			if (*p == '\r' || *p == '\n' || *p == ' ' || *p == '\t')
				continue;
			norm[n++] = (char)((*p >= 'A' && *p <= 'Z') ? *p - 'A' + 'a' : *p);
		}
		norm[n] = 0;
		// "centre" -> "center", anywhere in the token.
		if (char* c = strstr(norm, "centre"))
			memcpy(c, "center", 6);
		for (int i = 0; i < Anchor_COUNT; ++i)
			if (strcmp(norm, kAnchorNames[i]) == 0)
				return (Anchor)i;
		return Anchor_Free;
	}

	struct Entry
	{
		std::string name;
		Anchor anchor = Anchor_Free;
	};

	std::vector<Entry> g_entries;
	bool  g_enabled = true;
	float g_threshold = 24.0f;          // logical pixels, scaled by host DPI
	ImGuiID g_movingId = 0;             // window being dragged last frame
	Anchor  g_pending = Anchor_Free;    // where it would land if released now

	Entry* Find(const char* name)
	{
		for (Entry& e : g_entries)
			if (e.name == name)
				return &e;
		return nullptr;
	}

	Entry& FindOrAdd(const char* name)
	{
		if (Entry* e = Find(name))
			return *e;
		g_entries.push_back(Entry{ name, Anchor_Free });
		return g_entries.back();
	}

	// Top-left corner a window of `size` needs so that `a` lands on the target,
	// flush against the client area with no margin.
	ImVec2 AnchorPos(Anchor a, const ImVec2& origin, const ImVec2& client, const ImVec2& size)
	{
		const float left = origin.x;
		const float midX = origin.x + (client.x - size.x) * 0.5f;
		const float right = origin.x + client.x - size.x;
		const float top = origin.y;
		const float midY = origin.y + (client.y - size.y) * 0.5f;
		const float bottom = origin.y + client.y - size.y;
		switch (a)
		{
		case Anchor_TopLeft:      return ImVec2(left,  top);
		case Anchor_TopCenter:    return ImVec2(midX,  top);
		case Anchor_TopRight:     return ImVec2(right, top);
		case Anchor_LeftCenter:   return ImVec2(left,  midY);
		case Anchor_Center:       return ImVec2(midX,  midY);
		case Anchor_RightCenter:  return ImVec2(right, midY);
		case Anchor_BottomLeft:   return ImVec2(left,  bottom);
		case Anchor_BottomCenter: return ImVec2(midX,  bottom);
		case Anchor_BottomRight:  return ImVec2(right, bottom);
		default:                  return ImVec2(0, 0);
		}
	}

	// A window we are willing to move: top-level, on screen this frame, and not
	// one ImGui itself owns (popups, tooltips, child windows) or one that asked
	// not to be moved.
	bool Snappable(ImGuiWindow* w)
	{
		if (!w || !w->WasActive)
			return false;
		if (w->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup |
		                ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoMove))
			return false;
		if (w->RootWindow != w)
			return false;
		return true;
	}

	void DrawTargets(ImGuiWindow* w, const ImVec2& origin, const ImVec2& client, Anchor active)
	{
		ImDrawList* dl = ImGui::GetForegroundDrawList();
		// The theme's accent, so the guides read as part of the application
		// rather than as a debug overlay bolted onto it. Both weights are
		// derived from the one colour -- a hardcoded second shade here is how a
		// palette quietly stops being a palette.
		const ImVec4 a = D2Theme(D2Theme_Accent);
		const ImU32 faint = IM_COL32((int)(a.x * 255), (int)(a.y * 255), (int)(a.z * 255), 80);
		const ImU32 hot   = IM_COL32((int)(a.x * 255), (int)(a.y * 255), (int)(a.z * 255), 235);
		const float r = 3.0f * (D2Host_DpiScale() > 0.0f ? D2Host_DpiScale() : 1.0f);

		// A dot at each of the nine targets, so the grid is visible while you
		// drag instead of being something you have to know is there.
		for (int i = Anchor_TopLeft; i < Anchor_COUNT; ++i)
		{
			const ImVec2 p = AnchorPos((Anchor)i, origin, client, w->Size);
			const ImVec2 c(p.x + w->Size.x * 0.5f, p.y + w->Size.y * 0.5f);
			dl->AddCircleFilled(c, r, i == active ? hot : faint);
		}
		if (active != Anchor_Free)
		{
			const ImVec2 p = AnchorPos(active, origin, client, w->Size);
			dl->AddRect(p, ImVec2(p.x + w->Size.x, p.y + w->Size.y), hot, 0.0f, 0, 2.0f);
			char label[64];
			snprintf(label, sizeof(label), "snap: %s", kAnchorNames[active]);
			dl->AddText(ImVec2(p.x + 8.0f, p.y + 8.0f), hot, label);
		}
	}

	// ---- persisted settings ------------------------------------------------
	//
	// Two entry names, both reserved: "Settings" for the knobs and "Anchors" for
	// the per-window list. They are our own entry names inside our own type
	// block, so they cannot collide with a window title.
	void SettingsClearAll(ImGuiContext*, ImGuiSettingsHandler*)
	{
		g_entries.clear();
	}

	void* SettingsReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
	{
		if (name && strcmp(name, "Settings") == 0)
			return (void*)(intptr_t)1;
		if (name && strcmp(name, "Anchors") == 0)
			return (void*)(intptr_t)2;
		return nullptr;
	}

	void SettingsReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
	{
		const intptr_t which = (intptr_t)entry;
		if (which == 1)
		{
			int v = 0;
			float f = 0.0f;
			if      (sscanf_s(line, "Enabled=%d", &v) == 1)   g_enabled = v != 0;
			else if (sscanf_s(line, "Threshold=%f", &f) == 1) g_threshold = f > 0.0f ? f : 24.0f;
			return;
		}
		if (which != 2)
			return;
		// name=anchor, split on the LAST '=' so a window title containing one
		// survives the round trip.
		const char* eq = strrchr(line, '=');
		if (!eq || eq == line)
			return;
		std::string name(line, (size_t)(eq - line));
		const Anchor a = AnchorFromName(eq + 1);
		if (a != Anchor_Free)
			FindOrAdd(name.c_str()).anchor = a;
	}

	void SettingsWriteAll(ImGuiContext*, ImGuiSettingsHandler* h, ImGuiTextBuffer* buf)
	{
		buf->appendf("[%s][Settings]\n", h->TypeName);
		buf->appendf("Enabled=%d\n", g_enabled ? 1 : 0);
		buf->appendf("Threshold=%d\n", (int)g_threshold);
		buf->append("\n");
		buf->appendf("[%s][Anchors]\n", h->TypeName);
		for (const Entry& e : g_entries)
			if (e.anchor != Anchor_Free)
				buf->appendf("%s=%s\n", e.name.c_str(), kAnchorNames[e.anchor]);
		buf->append("\n");
	}

	// Numpad 1-9 place the FOCUSED window; 0 releases it. Numpad keys are not in
	// the game panel's forwarding table (D2Debugger.gamepanel.cpp kKeys), so
	// nothing here can also reach the game.
	Anchor HotkeyAnchor()
	{
		ImGuiIO& io = ImGui::GetIO();
		if (io.WantTextInput)
			return Anchor_COUNT;                       // typing: not ours
		struct Bind { ImGuiKey key; Anchor a; };
		static const Bind kBinds[] = {
			{ ImGuiKey_Keypad7, Anchor_TopLeft    }, { ImGuiKey_Keypad8, Anchor_TopCenter    }, { ImGuiKey_Keypad9, Anchor_TopRight    },
			{ ImGuiKey_Keypad4, Anchor_LeftCenter }, { ImGuiKey_Keypad5, Anchor_Center       }, { ImGuiKey_Keypad6, Anchor_RightCenter },
			{ ImGuiKey_Keypad1, Anchor_BottomLeft }, { ImGuiKey_Keypad2, Anchor_BottomCenter }, { ImGuiKey_Keypad3, Anchor_BottomRight },
			{ ImGuiKey_Keypad0, Anchor_Free       },
		};
		for (const Bind& b : kBinds)
			if (ImGui::IsKeyPressed(b.key, false))
				return b.a;
		return Anchor_COUNT;                           // nothing pressed
	}
}

// Called once per frame, immediately after ImGui::NewFrame() and BEFORE any
// panel is drawn. See the header for why the order matters.
void D2Snap_NewFrame()
{
	ImGuiContext* ctx = ImGui::GetCurrentContext();
	if (!ctx || !g_enabled)
		return;
	ImGuiContext& g = *ctx;

	ImGuiViewport* vp = ImGui::GetMainViewport();
	const ImVec2 origin = vp->Pos;
	const ImVec2 client = vp->Size;
	if (client.x <= 1.0f || client.y <= 1.0f)
		return;

	const float scale = D2Host_DpiScale() > 0.0f ? D2Host_DpiScale() : 1.0f;
	const float threshold = g_threshold * scale;

	// ---- the window being dragged -----------------------------------------
	ImGuiWindow* moving = g.MovingWindow ? g.MovingWindow->RootWindow : nullptr;
	if (moving && !Snappable(moving))
		moving = nullptr;

	if (moving)
	{
		// Nearest target to where the drag has actually put it. Distance is
		// measured between window ORIGINS, which is the same as measuring the
		// offset from a perfect snap -- so "near the bottom edge but way off to
		// the left" correctly matches nothing, rather than matching
		// bottom-centre because one axis happened to line up.
		Anchor best = Anchor_Free;
		float bestD2 = threshold * threshold;
		for (int i = Anchor_TopLeft; i < Anchor_COUNT; ++i)
		{
			const ImVec2 p = AnchorPos((Anchor)i, origin, client, moving->Size);
			const float dx = p.x - moving->Pos.x;
			const float dy = p.y - moving->Pos.y;
			const float d2 = dx * dx + dy * dy;
			if (d2 <= bestD2)
			{
				bestD2 = d2;
				best = (Anchor)i;
			}
		}
		g_movingId = moving->ID;
		g_pending = best;
		if (best != Anchor_Free)
			ImGui::SetWindowPos(moving, AnchorPos(best, origin, client, moving->Size),
			                    ImGuiCond_Always);
		DrawTargets(moving, origin, client, best);
	}
	else if (g_movingId != 0)
	{
		// The drag ended this frame: commit whatever it was hovering, including
		// Free, so dragging a window OFF an anchor is remembered as un-snapped
		// rather than silently re-pinning on the next launch.
		if (ImGuiWindow* w = ImGui::FindWindowByID(g_movingId))
		{
			Entry& e = FindOrAdd(w->Name);
			if (e.anchor != g_pending)
			{
				e.anchor = g_pending;
				ImGui::MarkIniSettingsDirty();
			}
		}
		g_movingId = 0;
		g_pending = Anchor_Free;
	}

	// ---- hotkeys -----------------------------------------------------------
	const Anchor pressed = HotkeyAnchor();
	if (pressed != Anchor_COUNT)
	{
		ImGuiWindow* target = g.NavWindow ? g.NavWindow->RootWindow : nullptr;
		if (Snappable(target))
		{
			Entry& e = FindOrAdd(target->Name);
			if (e.anchor != pressed)
			{
				e.anchor = pressed;
				ImGui::MarkIniSettingsDirty();
			}
		}
	}

	// ---- re-pin everything that is anchored --------------------------------
	//
	// Every frame, not once at startup: this is what keeps the game panel on its
	// anchor when the render resolution flips between the menu and in-world and
	// the panel resizes underneath, and what re-derives every anchor when the
	// host window changes size.
	for (ImGuiWindow* w : g.Windows)
	{
		if (w == moving || !Snappable(w))
			continue;
		Entry* e = Find(w->Name);
		if (!e || e->anchor == Anchor_Free)
			continue;
		const ImVec2 want = AnchorPos(e->anchor, origin, client, w->Size);
		if (w->Pos.x != want.x || w->Pos.y != want.y)
			ImGui::SetWindowPos(w, want, ImGuiCond_Always);
	}
}

// For a panel that wants to show its own snap state (the game panel does).
// Returns the canonical anchor name, "free" when unsnapped.
extern "C" const char* D2Snap_AnchorName(const char* window)
{
	const Entry* e = window ? Find(window) : nullptr;
	return kAnchorNames[e ? e->anchor : Anchor_Free];
}

void D2Snap_RegisterSettings()
{
	ImGuiSettingsHandler h;
	h.TypeName = "D2PanelSnap";
	h.TypeHash = ImHashStr("D2PanelSnap");
	h.ClearAllFn = SettingsClearAll;
	h.ReadOpenFn = SettingsReadOpen;
	h.ReadLineFn = SettingsReadLine;
	h.WriteAllFn = SettingsWriteAll;
	ImGui::AddSettingsHandler(&h);
}
