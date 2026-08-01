// D2Debugger.theme.cpp -- a dark theme: black, greys and cold blues.
//
// THE CHROME IS NEUTRAL. Backgrounds run from true black through cool dark
// greys, borders are slate, and the accent is a steel blue. Nothing in the
// frame is warm: an earlier pass tinted the darks brown with gold accents to
// echo D2's lamplit stone, and against the actual game it read as a costume
// rather than as dark mode. A tool you keep open for hours should be the quiet
// thing on the screen.
//
// THE CONTENT IS WHERE DIABLO SHOWS UP. The semantic colours are still taken
// from the game's own item-quality palette, because those six colours already
// meant almost exactly these six things and are instantly readable to anyone
// who has played it:
//
//     set green     #00FF00   proven / live / clean          (softened; raw
//                             0,255,0 vibrates on a dark background)
//     magic blue    #6969FF   identifiers and function names
//     rare yellow   #FFFF64   proven offline only -- not yet observed live
//     crafted amber #FFA800   not hooked, missing, disabled
//     blood red     #A82119   divergence and failure
//
// Keeping those on a neutral frame is what makes them legible AS status: on the
// brown-and-gold frame the amber and yellow were competing with the chrome.
//
// TWO BLUES, ON PURPOSE, and they must not converge. The ACCENT (steel, dark
// and desaturated) is chrome -- focused title bar, checkmarks, grips. MAGIC
// BLUE (bright, saturated) is content -- it means "this is an identifier". If
// the accent is ever brightened toward the magic blue, an active title bar
// starts reading as data.
//
// StyleColorsDark is applied FIRST and then overridden, deliberately: any
// ImGuiCol_ this file has not heard of (imgui adds them) still gets a sane dark
// value instead of an uninitialised one.

#include "D2Debugger.theme.h"

namespace
{
	// The palette, once. Everything below is expressed in these.
	constexpr ImVec4 kBlack      { 0.000f, 0.000f, 0.000f, 1.00f };  // #000000 the void behind the panels
	constexpr ImVec4 kInk        { 0.043f, 0.049f, 0.059f, 1.00f };  // #0B0C0F child/inset backgrounds
	constexpr ImVec4 kSlate      { 0.063f, 0.071f, 0.086f, 1.00f };  // #101216 window body
	constexpr ImVec4 kSlateLit   { 0.090f, 0.102f, 0.125f, 1.00f };  // #171A20 inputs, frames, title bar
	constexpr ImVec4 kSlateHi    { 0.129f, 0.149f, 0.184f, 1.00f };  // #21262F hover
	constexpr ImVec4 kSlateSel   { 0.173f, 0.200f, 0.247f, 1.00f };  // #2C333F active / selected
	constexpr ImVec4 kEdge       { 0.184f, 0.208f, 0.259f, 1.00f };  // #2F3542 borders
	constexpr ImVec4 kSteel      { 0.290f, 0.435f, 0.647f, 1.00f };  // #4A6FA5 accent
	constexpr ImVec4 kSteelHi    { 0.431f, 0.592f, 0.839f, 1.00f };  // #6E97D6 accent, hovered/active
	constexpr ImVec4 kText       { 0.816f, 0.831f, 0.855f, 1.00f };  // #D0D4DA body text, cool grey
	constexpr ImVec4 kTextDim    { 0.420f, 0.447f, 0.494f, 1.00f };  // #6B727E disabled / zero

	// Item-quality colours -- content, not chrome.
	constexpr ImVec4 kSetGreen   { 0.360f, 0.820f, 0.360f, 1.00f };  // set item, softened
	constexpr ImVec4 kSetGreenHi { 0.480f, 0.930f, 0.480f, 1.00f };  // live hits
	constexpr ImVec4 kMagicBlue  { 0.560f, 0.580f, 0.960f, 1.00f };  // #6969FF, lifted for contrast
	constexpr ImVec4 kRareYellow { 0.930f, 0.870f, 0.400f, 1.00f };  // #FFFF64, softened
	constexpr ImVec4 kAmber      { 0.940f, 0.640f, 0.180f, 1.00f };  // #FFA800 crafted
	constexpr ImVec4 kBlood      { 0.850f, 0.290f, 0.260f, 1.00f };  // #A82119, lifted to stay legible

	ImVec4 WithAlpha(const ImVec4& c, float a)
	{
		return ImVec4(c.x, c.y, c.z, a);
	}
}

ImVec4 D2Theme(D2ThemeCol c)
{
	switch (c)
	{
	case D2Theme_Text:   return kText;
	case D2Theme_Dim:    return kTextDim;
	case D2Theme_Ok:     return kSetGreen;
	case D2Theme_Hot:    return kSetGreenHi;
	case D2Theme_Warn:   return kRareYellow;
	case D2Theme_Alert:  return kAmber;
	case D2Theme_Bad:    return kBlood;
	case D2Theme_Name:   return kMagicBlue;
	case D2Theme_Accent: return kSteelHi;
	default:             return kText;
	}
}

void D2Theme_ClearColor(float* rgba4)
{
	if (!rgba4)
		return;
	// BLACK, not a dark grey. This is what shows where no panel is, so on a
	// borderless 2560x1440 surface it is by area the most visible colour in the
	// application -- and the panels read as floating on nothing rather than as
	// sitting on a slab.
	rgba4[0] = kBlack.x;
	rgba4[1] = kBlack.y;
	rgba4[2] = kBlack.z;
	rgba4[3] = 1.0f;
}

void D2Theme_Apply()
{
	ImGuiStyle& s = ImGui::GetStyle();
	ImGui::StyleColorsDark(&s);          // sane floor for anything not set below
	ImVec4* c = s.Colors;

	c[ImGuiCol_Text]                  = kText;
	c[ImGuiCol_TextDisabled]          = kTextDim;
	c[ImGuiCol_WindowBg]              = kSlate;
	c[ImGuiCol_ChildBg]               = WithAlpha(kInk, 0.60f);
	c[ImGuiCol_PopupBg]               = WithAlpha(kInk, 0.98f);
	c[ImGuiCol_Border]                = kEdge;
	c[ImGuiCol_BorderShadow]          = ImVec4(0, 0, 0, 0);
	c[ImGuiCol_FrameBg]               = kSlateLit;
	c[ImGuiCol_FrameBgHovered]        = kSlateHi;
	c[ImGuiCol_FrameBgActive]         = kSlateSel;

	// The title bar is the one place the accent is allowed to be loud: it is
	// how you tell the focused panel from the others behind it. Kept dark and
	// desaturated so it never competes with magic-blue identifiers.
	c[ImGuiCol_TitleBg]               = kSlateLit;
	c[ImGuiCol_TitleBgActive]         = ImVec4(0.153f, 0.235f, 0.353f, 1.00f);  // #27385A
	c[ImGuiCol_TitleBgCollapsed]      = WithAlpha(kSlateLit, 0.80f);
	c[ImGuiCol_MenuBarBg]             = kSlateLit;

	c[ImGuiCol_ScrollbarBg]           = WithAlpha(kBlack, 0.45f);
	c[ImGuiCol_ScrollbarGrab]         = kSlateSel;
	c[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.235f, 0.290f, 0.365f, 1.00f);
	c[ImGuiCol_ScrollbarGrabActive]   = kSteel;

	c[ImGuiCol_CheckMark]             = kSteelHi;
	c[ImGuiCol_SliderGrab]            = kSteel;
	c[ImGuiCol_SliderGrabActive]      = kSteelHi;

	c[ImGuiCol_Button]                = ImVec4(0.129f, 0.153f, 0.192f, 1.00f);
	c[ImGuiCol_ButtonHovered]         = ImVec4(0.196f, 0.243f, 0.310f, 1.00f);
	c[ImGuiCol_ButtonActive]          = ImVec4(0.255f, 0.325f, 0.424f, 1.00f);

	c[ImGuiCol_Header]                = ImVec4(0.141f, 0.169f, 0.212f, 1.00f);
	c[ImGuiCol_HeaderHovered]         = ImVec4(0.204f, 0.255f, 0.325f, 1.00f);
	c[ImGuiCol_HeaderActive]          = ImVec4(0.259f, 0.329f, 0.427f, 1.00f);

	c[ImGuiCol_Separator]             = kEdge;
	c[ImGuiCol_SeparatorHovered]      = kSteel;
	c[ImGuiCol_SeparatorActive]       = kSteelHi;

	c[ImGuiCol_ResizeGrip]            = WithAlpha(kEdge, 0.80f);
	c[ImGuiCol_ResizeGripHovered]     = kSteel;
	c[ImGuiCol_ResizeGripActive]      = kSteelHi;

	c[ImGuiCol_Tab]                   = ImVec4(0.106f, 0.125f, 0.157f, 1.00f);
	c[ImGuiCol_TabHovered]            = ImVec4(0.204f, 0.267f, 0.353f, 1.00f);
	c[ImGuiCol_TabSelected]           = ImVec4(0.161f, 0.216f, 0.298f, 1.00f);
	c[ImGuiCol_TabSelectedOverline]   = kSteelHi;
	c[ImGuiCol_TabDimmed]             = ImVec4(0.078f, 0.090f, 0.114f, 1.00f);
	c[ImGuiCol_TabDimmedSelected]     = ImVec4(0.118f, 0.145f, 0.188f, 1.00f);

	c[ImGuiCol_PlotLines]             = kSteelHi;
	c[ImGuiCol_PlotLinesHovered]      = kText;
	c[ImGuiCol_PlotHistogram]         = kSteel;
	c[ImGuiCol_PlotHistogramHovered]  = kSteelHi;

	c[ImGuiCol_TableHeaderBg]         = ImVec4(0.118f, 0.137f, 0.173f, 1.00f);
	c[ImGuiCol_TableBorderStrong]     = kEdge;
	c[ImGuiCol_TableBorderLight]      = ImVec4(0.145f, 0.165f, 0.204f, 1.00f);
	c[ImGuiCol_TableRowBg]            = ImVec4(0, 0, 0, 0);
	c[ImGuiCol_TableRowBgAlt]         = WithAlpha(kText, 0.030f);

	c[ImGuiCol_TextSelectedBg]        = WithAlpha(kSteel, 0.45f);
	c[ImGuiCol_DragDropTarget]        = kSteelHi;
	c[ImGuiCol_NavCursor]             = kSteelHi;
	c[ImGuiCol_NavWindowingHighlight] = WithAlpha(kSteelHi, 0.70f);
	c[ImGuiCol_NavWindowingDimBg]     = WithAlpha(kBlack, 0.60f);
	c[ImGuiCol_ModalWindowDimBg]      = WithAlpha(kBlack, 0.70f);

	// Geometry. Small rounding and visible borders: with backgrounds this close
	// to black, the border is often the only thing separating one panel from
	// the next. Base values only -- these are what ApplyUiScale multiplies for
	// the monitor's DPI, so putting scaled numbers here would double-apply.
	s.WindowRounding    = 3.0f;
	s.ChildRounding     = 3.0f;
	s.FrameRounding     = 3.0f;
	s.PopupRounding     = 3.0f;
	s.ScrollbarRounding = 3.0f;
	s.GrabRounding      = 3.0f;
	s.TabRounding       = 3.0f;
	s.WindowBorderSize  = 1.0f;
	s.ChildBorderSize   = 1.0f;
	s.FrameBorderSize   = 1.0f;
	s.PopupBorderSize   = 1.0f;
	s.WindowTitleAlign  = ImVec2(0.0f, 0.5f);
}
