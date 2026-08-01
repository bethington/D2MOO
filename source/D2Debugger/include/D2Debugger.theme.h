// D2Debugger.theme.h -- the debugger's dark theme, and the ONE place the
// semantic colours live.
//
// The panels had a working colour vocabulary before this file existed -- green
// for observed-live, yellow for offline-proven-only, red for divergence, orange
// for no-hook -- but it was spelled out as literal ImVec4s at nineteen call
// sites, in four slightly different shades of the same idea. Naming the
// MEANING here (and the Diablo II colour it maps to) means the palette can be
// retuned once instead of nineteen times, and a new panel gets the same green
// as every other panel by asking for the same name.
#pragma once

#include "imgui.h"

// Semantic roles, not colour names. Call sites should say what a thing MEANS
// (this function diverged) rather than what it looks like (this is red).
enum D2ThemeCol
{
	D2Theme_Text,       // ordinary text -- cool light grey rather than white
	D2Theme_Dim,        // present but unremarkable: zero hits, not applicable
	D2Theme_Ok,         // proven, live, clean            -- set-item green
	D2Theme_Hot,        // live activity, hit counts      -- brighter set green
	D2Theme_Warn,       // proven offline only, degraded  -- rare-item yellow
	D2Theme_Alert,      // not hooked, missing, disabled  -- crafted orange
	D2Theme_Bad,        // divergence, failure            -- blood red
	D2Theme_Name,       // identifiers / function names   -- magic blue
	D2Theme_Accent,     // chrome emphasis: guides, focus    -- steel blue
	D2Theme_COUNT
};

// Apply the whole style: colours, rounding, borders. Call once, after the ImGui
// context exists and BEFORE any DPI scaling of sizes -- ScaleAllSizes multiplies
// whatever it finds, so it has to run on the theme's own base numbers.
void D2Theme_Apply();

// One semantic colour.
ImVec4 D2Theme(D2ThemeCol c);

// The colour the D3D9 backbuffer is cleared to, i.e. what you see where no
// panel is. Part of the theme because the default was a pale blue-grey that
// was, by area, the single most visible thing on a 2560x1440 surface.
void D2Theme_ClearColor(float* rgba4);
