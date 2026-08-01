// D2Debugger.canvas.h -- the panel's FIXED drawing surface and what fills the
// part of it the game is not currently using.
//
// THE PROBLEM. D2 does not render the menu at the resolution it plays at. The
// menu comes through as 800x600 and the splash as 640x480, while the game world
// is whatever ddraw.ini declares -- 1068x600 here. With the panel sized from the
// live frame, entering a game resized the window under you, and so did leaving
// one.
//
// So the panel is sized from the CANVAS -- the resolution the game is set to --
// and the live frame is drawn into it at 1:1, centred, never scaled. The canvas
// is constant across menu, character select and world, so the window stops
// moving. What is left over around the frame is the extension, below.
#pragma once

#include "imgui.h"

// Read the configured resolution. Call once, before the first frame.
void D2Canvas_Init();

// The canvas size in pixels -- what the panel should be sized to.
void D2Canvas_Size(int* w, int* h);

// Observe a frame. A frame LARGER than the canvas grows it: ddraw.ini is
// advisory (cnc-ddraw rewrites it on exit, and a mod may render wider than it
// declares), and a frame we cannot show is a worse failure than a slightly big
// window.
void D2Canvas_NoteFrame(int w, int h);

// Feed the procedural filler. MUST be called with the capture lock held, since
// it reads the frame's pixels -- it samples a small fixed grid rather than
// copying, so the game's render thread is blocked for microseconds, and it
// rate-limits itself internally.
void D2Canvas_SampleFrame(const unsigned char* rgba, int w, int h);

// Draw the area around the frame. `origin` is the canvas's top-left in screen
// space; the caller draws the live frame on top afterwards.
void D2Canvas_DrawBackdrop(ImDrawList* dl, const ImVec2& origin, int frameW, int frameH);

// Where the canvas size came from, for the status line ("ddraw.ini", "observed").
const char* D2Canvas_SourceName();

// Which filler is actually in use right now ("baked", "procedural", "none").
const char* D2Canvas_FillName();

// Before IDirect3DDevice9::Reset -- the textures live in D3DPOOL_DEFAULT.
void D2Canvas_ReleaseDeviceObjects();
