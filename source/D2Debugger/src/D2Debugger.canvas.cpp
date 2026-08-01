// D2Debugger.canvas.cpp -- a fixed-size drawing surface, and the extension that
// fills it around an undersized frame.
//
// THE CANVAS comes from ddraw.ini's width/height, which is the resolution the
// game is configured to play at (1068x600 here) and the one thing that does NOT
// change between the splash, the menu and the world. A frame bigger than the
// canvas grows it, because a frame we cannot show is a worse failure than a
// window slightly larger than expected.
//
// THE EXTENSION, in priority order:
//
//   1. BAKED ART.  <ext dir>/<canvas>.<frame>.png, then <ext dir>/<canvas>.png.
//      Drawn untinted, so a properly outpainted image is shown exactly as
//      authored. The two-step lookup means one generic backdrop covers every
//      menu screen -- D2's menus are all dark and vignetted at the edges -- and
//      a specific source resolution (the 640x480 splash) can override it
//      without touching the generic one.
//
//   2. PROCEDURAL.  The frame's own edges, MIRRORED outward, blurred and
//      darkened. Works on every screen forever, including ones PD2 has not
//      shipped yet, and needs no assets.
//
//      The blur is free: rather than blurring a 1068x600 image, the mirrored
//      result is built at 64x36 and drawn stretched across the canvas. The
//      GPU's bilinear filter IS the blur. That is ~2300 pixel reads per
//      rebuild instead of 640,000, which is why this can be sampled straight
//      out of the locked frame without the game's render thread noticing.
//
//   3. NOTHING. Whatever the panel's background is -- the honest black bars.
//
// PNG loading goes through WIC, which ships with Windows. stb_image would have
// meant vendoring a 7,700-line header for one call, and WIC additionally reads
// JPG/BMP/TIFF, so a dropped-in asset is unlikely to be rejected for its format.

#include "D2Debugger.canvas.h"

#include <windows.h>
#include <d3d9.h>
#include <wincodec.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

LPDIRECT3DDEVICE9 D2Panel_GetDevice();

namespace
{
	// Procedural grid. Deliberately tiny -- see the blur note in the header.
	constexpr int kGridW = 64;
	constexpr int kGridH = 36;
	// How often the procedural fill may be rebuilt. The menu background is
	// static apart from the fire, so this only needs to be fast enough to catch
	// a SCREEN change (title -> character select), not to track animation.
	constexpr unsigned kRebuildMs = 250;
	// How much to darken the procedural fill. It has to read as surround rather
	// than as content, or the eye keeps being pulled off the actual game.
	constexpr float kProcTint = 0.42f;

	int  g_canvasW = 0, g_canvasH = 0;
	char g_source[32] = "default";

	// Procedural
	LPDIRECT3DTEXTURE9 g_proc = nullptr;
	unsigned char g_grid[kGridW * kGridH * 4];
	bool g_gridReady = false;
	bool g_gridDirty = false;
	unsigned g_lastSample = 0;
	int g_sampledFrameW = 0, g_sampledFrameH = 0;

	// Baked
	LPDIRECT3DTEXTURE9 g_baked = nullptr;
	int g_bakedW = 0, g_bakedH = 0;
	std::string g_bakedKey;          // key the current baked texture was loaded for
	std::string g_triedKey;          // key we last attempted, so a miss is not retried per frame
	const char* g_fill = "none";

	void CanvasLog(const char* fmt, ...)
	{
		char buf[512];
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);
		char line[600];
		snprintf(line, sizeof(line), "[canvas] %s\n", buf);
		OutputDebugStringA(line);
		FILE* f = nullptr;
		if (fopen_s(&f, "C:\\Users\\benam\\source\\cpp\\D2MOO\\conformance\\behavioral\\overlay_gl_log.txt", "a") == 0 && f)
		{
			fputs(line, f);
			fclose(f);
		}
	}

	// The directory Game.exe lives in.
	//
	// NOT the working directory. Measured: at the point this runs, the process
	// CWD is C:\WINDOWS\system32 -- the launcher starts the game from there and
	// the game sets its own CWD later during init. So a relative "ddraw.ini"
	// resolved to system32, silently missed, and the canvas fell back to a
	// default 800x600 that looked exactly like "ddraw.ini says 800x600".
	// (imgui.ini lands in the game folder only because ImGui writes it lazily,
	// by which time the CWD has moved -- which is precisely the sort of
	// coincidence that makes a CWD-relative path look like it works.)
	std::string GameDir()
	{
		char exe[MAX_PATH]{};
		if (!GetModuleFileNameA(nullptr, exe, sizeof(exe)))
			return ".";
		if (char* slash = strrchr(exe, '\\'))
			*slash = 0;
		return std::string(exe);
	}

	std::string ExtDir()
	{
		char buf[MAX_PATH]{};
		if (GetEnvironmentVariableA("D2DBG_EXT_DIR", buf, sizeof(buf)) && buf[0])
			return std::string(buf);
		return GameDir() + "\\d2dbg_ext";
	}

	// ---- ddraw.ini ---------------------------------------------------------
	//
	// Line-based and forgiving: cnc-ddraw's ini carries comments and dozens of
	// keys we have no business understanding, and this only wants two of them.
	bool ReadDdrawIni(int* w, int* h)
	{
		std::string path;
		char env[MAX_PATH]{};
		if (GetEnvironmentVariableA("D2_DDRAW_INI", env, sizeof(env)) && env[0])
			path = env;                            // same override fun-doc uses
		else
			path = GameDir() + "\\ddraw.ini";      // beside Game.exe, not beside the CWD

		FILE* f = nullptr;
		if (fopen_s(&f, path.c_str(), "r") != 0 || !f)
		{
			CanvasLog("ddraw.ini not readable at %s", path.c_str());
			return false;
		}
		int gotW = 0, gotH = 0;
		char line[256];
		while (fgets(line, sizeof(line), f))
		{
			int v = 0;
			// Leading whitespace only; a commented '; width=' must not match.
			const char* p = line;
			while (*p == ' ' || *p == '\t') ++p;
			if (sscanf_s(p, "width=%d", &v) == 1 && v > 0)  gotW = v;
			else if (sscanf_s(p, "height=%d", &v) == 1 && v > 0) gotH = v;
		}
		fclose(f);
		if (gotW <= 0 || gotH <= 0)
			return false;
		*w = gotW;
		*h = gotH;
		return true;
	}

	void ReleaseProc()
	{
		if (g_proc) { g_proc->Release(); g_proc = nullptr; }
	}

	void ReleaseBaked()
	{
		if (g_baked) { g_baked->Release(); g_baked = nullptr; }
		g_bakedW = g_bakedH = 0;
		g_bakedKey.clear();
	}

	// Reflect an out-of-range coordinate back into [0, n). Mirroring rather than
	// clamping matters: clamping smears the single edge column into a streak,
	// which looks like a rendering fault; mirroring continues the picture.
	int Mirror(int v, int n)
	{
		if (n <= 1)
			return 0;
		while (v < 0 || v >= n)
		{
			if (v < 0)  v = -v;
			if (v >= n) v = 2 * (n - 1) - v;
		}
		return v;
	}

	// Upload g_grid into a texture. Off the capture lock -- the sampling under
	// the lock only fills the CPU-side grid.
	bool EnsureProcTexture()
	{
		if (!g_gridReady)
			return false;
		LPDIRECT3DDEVICE9 dev = D2Panel_GetDevice();
		if (!dev)
			return false;
		if (!g_proc)
		{
			if (FAILED(dev->CreateTexture(kGridW, kGridH, 1, D3DUSAGE_DYNAMIC,
			                              D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
			                              &g_proc, nullptr)))
				return false;
			g_gridDirty = true;
		}
		if (g_gridDirty)
		{
			D3DLOCKED_RECT lr{};
			if (SUCCEEDED(g_proc->LockRect(0, &lr, nullptr, D3DLOCK_DISCARD)))
			{
				for (int y = 0; y < kGridH; ++y)
				{
					const unsigned char* s = g_grid + (size_t)y * kGridW * 4;
					unsigned char* d = (unsigned char*)lr.pBits + (size_t)y * lr.Pitch;
					memcpy(d, s, (size_t)kGridW * 4);
				}
				g_proc->UnlockRect(0);
				g_gridDirty = false;
			}
		}
		return g_proc != nullptr;
	}

	// ---- baked art ---------------------------------------------------------
	LPDIRECT3DTEXTURE9 LoadPng(const char* path, int* outW, int* outH)
	{
		LPDIRECT3DDEVICE9 dev = D2Panel_GetDevice();
		if (!dev)
			return nullptr;

		// Our own thread, our own apartment. RPC_E_CHANGED_MODE means someone
		// already initialised it differently, which is fine -- the factory works
		// either way, and we must NOT balance that case with CoUninitialize.
		const HRESULT coInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

		IWICImagingFactory* factory = nullptr;
		IWICBitmapDecoder* dec = nullptr;
		IWICBitmapFrameDecode* frame = nullptr;
		IWICFormatConverter* conv = nullptr;
		LPDIRECT3DTEXTURE9 tex = nullptr;

		do
		{
			if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
			                            IID_PPV_ARGS(&factory))))
				break;
			wchar_t wpath[MAX_PATH]{};
			MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_PATH);
			if (FAILED(factory->CreateDecoderFromFilename(wpath, nullptr, GENERIC_READ,
			                                              WICDecodeMetadataCacheOnLoad, &dec)))
				break;
			if (FAILED(dec->GetFrame(0, &frame)))
				break;
			if (FAILED(factory->CreateFormatConverter(&conv)))
				break;
			// 32bppBGRA is exactly D3DFMT_A8R8G8B8's memory order, so the copy
			// below is a straight memcpy per row with no channel shuffling.
			if (FAILED(conv->Initialize(frame, GUID_WICPixelFormat32bppBGRA,
			                            WICBitmapDitherTypeNone, nullptr, 0.0,
			                            WICBitmapPaletteTypeCustom)))
				break;
			UINT w = 0, h = 0;
			if (FAILED(conv->GetSize(&w, &h)) || w == 0 || h == 0)
				break;

			std::vector<unsigned char> pixels((size_t)w * h * 4);
			if (FAILED(conv->CopyPixels(nullptr, w * 4, (UINT)pixels.size(), pixels.data())))
				break;
			if (FAILED(dev->CreateTexture(w, h, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8,
			                              D3DPOOL_DEFAULT, &tex, nullptr)))
				break;
			D3DLOCKED_RECT lr{};
			if (FAILED(tex->LockRect(0, &lr, nullptr, D3DLOCK_DISCARD)))
			{
				tex->Release();
				tex = nullptr;
				break;
			}
			for (UINT y = 0; y < h; ++y)
				memcpy((unsigned char*)lr.pBits + (size_t)y * lr.Pitch,
				       pixels.data() + (size_t)y * w * 4, (size_t)w * 4);
			tex->UnlockRect(0);
			*outW = (int)w;
			*outH = (int)h;
		} while (false);

		if (conv)    conv->Release();
		if (frame)   frame->Release();
		if (dec)     dec->Release();
		if (factory) factory->Release();
		if (SUCCEEDED(coInit))
			CoUninitialize();
		return tex;
	}

	// Try <canvas>.<frame>.png, then <canvas>.png. A miss is remembered so the
	// disk is not hit every frame for art that is not there -- which is the
	// normal case until someone authors some.
	void EnsureBaked(int frameW, int frameH)
	{
		char key[64];
		snprintf(key, sizeof(key), "%dx%d|%dx%d", g_canvasW, g_canvasH, frameW, frameH);
		if (g_bakedKey == key || g_triedKey == key)
			return;
		g_triedKey = key;
		ReleaseBaked();

		const std::string dir = ExtDir();
		char specific[MAX_PATH], generic[MAX_PATH];
		snprintf(specific, sizeof(specific), "%s\\%dx%d.%dx%d.png",
		         dir.c_str(), g_canvasW, g_canvasH, frameW, frameH);
		snprintf(generic, sizeof(generic), "%s\\%dx%d.png",
		         dir.c_str(), g_canvasW, g_canvasH);

		const char* picked = nullptr;
		if (GetFileAttributesA(specific) != INVALID_FILE_ATTRIBUTES)
			picked = specific;
		else if (GetFileAttributesA(generic) != INVALID_FILE_ATTRIBUTES)
			picked = generic;
		if (!picked)
			return;

		int w = 0, h = 0;
		if (LPDIRECT3DTEXTURE9 t = LoadPng(picked, &w, &h))
		{
			g_baked = t;
			g_bakedW = w;
			g_bakedH = h;
			g_bakedKey = key;
			CanvasLog("baked extension loaded: %s (%dx%d)", picked, w, h);
		}
		else
		{
			CanvasLog("baked extension FAILED to load: %s", picked);
		}
	}
}

void D2Canvas_Init()
{
	int w = 0, h = 0;
	if (ReadDdrawIni(&w, &h))
	{
		g_canvasW = w;
		g_canvasH = h;
		strcpy_s(g_source, "ddraw.ini");
	}
	else
	{
		// Not a guess we have to be right about: the first frame observed will
		// grow this, and D2's own default is 800x600.
		g_canvasW = 800;
		g_canvasH = 600;
		strcpy_s(g_source, "default");
	}
	CanvasLog("canvas %dx%d from %s; extension dir %s",
	          g_canvasW, g_canvasH, g_source, ExtDir().c_str());
}

void D2Canvas_Size(int* w, int* h)
{
	if (w) *w = g_canvasW;
	if (h) *h = g_canvasH;
}

void D2Canvas_NoteFrame(int w, int h)
{
	if (w <= 0 || h <= 0)
		return;
	if (w > g_canvasW || h > g_canvasH)
	{
		CanvasLog("frame %dx%d exceeds canvas %dx%d -- growing", w, h, g_canvasW, g_canvasH);
		if (w > g_canvasW) g_canvasW = w;
		if (h > g_canvasH) g_canvasH = h;
		strcpy_s(g_source, "observed");
		g_triedKey.clear();          // canvas changed: re-look for baked art
		ReleaseBaked();
	}
}

void D2Canvas_SampleFrame(const unsigned char* rgba, int w, int h)
{
	if (!rgba || w <= 0 || h <= 0 || g_canvasW <= 0)
		return;
	// Nothing to extend when the frame already covers the canvas -- and the
	// in-world case is exactly when the game's render thread is busiest.
	if (w >= g_canvasW && h >= g_canvasH)
		return;

	const unsigned now = GetTickCount();
	const bool sizeChanged = (w != g_sampledFrameW || h != g_sampledFrameH);
	if (!sizeChanged && g_gridReady && (now - g_lastSample) < kRebuildMs)
		return;
	g_lastSample = now;
	g_sampledFrameW = w;
	g_sampledFrameH = h;

	const int offX = (g_canvasW - w) / 2;
	const int offY = (g_canvasH - h) / 2;

	for (int gy = 0; gy < kGridH; ++gy)
	{
		// Grid cell centre -> canvas pixel -> frame pixel.
		const int cy = (int)(((float)gy + 0.5f) / kGridH * (float)g_canvasH);
		const int fy = Mirror(cy - offY, h);
		for (int gx = 0; gx < kGridW; ++gx)
		{
			const int cx = (int)(((float)gx + 0.5f) / kGridW * (float)g_canvasW);
			const int fx = Mirror(cx - offX, w);
			const unsigned char* s = rgba + ((size_t)fy * w + fx) * 4;
			unsigned char* d = g_grid + ((size_t)gy * kGridW + gx) * 4;
			// Source is R,G,B,A; D3DFMT_A8R8G8B8 is B,G,R,A in memory -- the
			// same swap the frame upload does.
			d[0] = s[2];
			d[1] = s[1];
			d[2] = s[0];
			d[3] = 255;
		}
	}
	g_gridReady = true;
	g_gridDirty = true;
}

void D2Canvas_DrawBackdrop(ImDrawList* dl, const ImVec2& origin, int frameW, int frameH)
{
	g_fill = "none";
	if (!dl || g_canvasW <= 0 || g_canvasH <= 0)
		return;
	if (frameW >= g_canvasW && frameH >= g_canvasH)
		return;                       // frame covers the canvas; nothing to fill

	const ImVec2 p0 = origin;
	const ImVec2 p1(origin.x + (float)g_canvasW, origin.y + (float)g_canvasH);

	EnsureBaked(frameW, frameH);
	if (g_baked)
	{
		// Authored art is shown as authored: no tint, no darkening.
		dl->AddImage((ImTextureID)g_baked, p0, p1);
		g_fill = "baked";
		return;
	}

	if (EnsureProcTexture())
	{
		// Half-texel inset. Without it the bilinear filter blends the outermost
		// texels against the clamp edge and the extension fades out at exactly
		// the place it is meant to meet the panel border.
		const ImVec2 uv0(0.5f / kGridW, 0.5f / kGridH);
		const ImVec2 uv1(1.0f - 0.5f / kGridW, 1.0f - 0.5f / kGridH);
		const ImU32 tint = IM_COL32((int)(255 * kProcTint), (int)(255 * kProcTint),
		                            (int)(255 * kProcTint), 255);
		dl->AddImage((ImTextureID)g_proc, p0, p1, uv0, uv1, tint);
		g_fill = "procedural";
	}
}

const char* D2Canvas_SourceName() { return g_source; }
const char* D2Canvas_FillName()   { return g_fill; }

void D2Canvas_ReleaseDeviceObjects()
{
	ReleaseProc();
	ReleaseBaked();
	// Both will be recreated on demand; the baked one has to be re-looked-up,
	// so forget the "already tried and missed" memo too.
	g_triedKey.clear();
	g_gridDirty = true;
}
