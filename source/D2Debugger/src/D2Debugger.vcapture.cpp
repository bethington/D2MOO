// D2Debugger.vcapture.cpp -- FRAME CAPTURE off the GDI present path.
//
// HOW THE FRAME ACTUALLY REACHES THE SCREEN (measured 2026-07-31)
// --------------------------------------------------------------
// The present-path probe (D2Debugger.probe.cpp) settled this empirically after
// four documented candidates were hooked, confirmed attached, and provably
// never fired:
//
//     StretchBlt        49/sec   <- from D2Gdi.dll, exactly the frame rate
//     BitBlt / StretchDIBits / SetDIBitsToDevice / PatBlt        0
//     wglSwapBuffers / wglSwapLayerBuffers / gdi32!SwapBuffers   0
//     glFlush / glFinish / glDrawPixels / DwmFlush               0
//
// So PD2 is NOT rendering with OpenGL. It uses D2's GDI software renderer.
// opengl32, glide3x and ddraw are all LOADED but idle -- which is why
// ddraw.ini's renderer=opengl looked authoritative and was not, and why
// D2Debugger.overlay.gl.cpp (built on "both present via wglSwapBuffers/
// SwapBuffers") was written, never wired up, and abandoned.
//
// D2Gdi.dll imports exactly four things, which spell out the classic GDI
// software-renderer pattern:
//
//     CreateDIBSection . CreateCompatibleDC . SelectObject . StretchBlt
//
// THE CONSEQUENCE, AND WHY THIS IS BETTER THAN THE GL PLAN
// --------------------------------------------------------
// CreateDIBSection hands back a DIRECT POINTER to the pixel buffer. The game
// rasterises into ordinary system memory and blits that to the window. So the
// finished frame is readable BY REFERENCE -- no glReadPixels, no GPU round
// trip, no pipeline stall, and no per-frame cost at all. Reading it is a
// pointer dereference.
//
// WHICH DIB IS THE FRAME
// We do not guess. At StretchBlt time the SOURCE DC holds the bitmap being
// presented, so GetCurrentObject(hdcSrc, OBJ_BITMAP) names it exactly; that
// HBITMAP is looked up in the small registry CreateDIBSection populates. A
// game that creates several DIBs therefore cannot confuse us.
//
// PIXEL FORMAT (measured): every D2 surface is 8bpp PALETTIZED and top-down --
// 640x480, 800x600 and the 1068x600 frame. So a capture is an index-to-RGBA
// expansion through the colour table read from the blit's source DC, not a
// straight copy. Getting this wrong yields a psychedelic image that still looks
// like a capture, which is why an unsupported format is refused outright
// instead of guessed at.
//
// A capture still takes ONE pass, deliberately: the DIB is live memory the
// game keeps drawing into, so encoding straight from it would tear. The copy
// happens only when a capture was requested; idle frames cost nothing.
#include <Windows.h>
#include <atomic>
#include <mutex>
#include <vector>
#include <cstdio>
#include <cstring>

// STATIC: imgui_draw.cpp already compiles a copy of stb_image_write into the
// imgui lib we link. Internal linkage here keeps the two from colliding.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"

namespace
{
	struct Dib
	{
		HBITMAP bmp = nullptr;
		void* bits = nullptr;
		int   w = 0, h = 0;      // h > 0 bottom-up, h < 0 top-down (as supplied)
		int   bpp = 0;
		// 8bpp is the D2 case: the surface holds palette INDICES, so the colour
		// table is needed to make sense of a single byte. Read from the source
		// DC at blit time via GetDIBColorTable.
		RGBQUAD pal[256]{};
		bool    havePal = false;
		// True when the presenting StretchBlt itself flips vertically (signed
		// dest/src heights disagree). Independent of the DIB's own orientation.
		bool    blitFlips = false;
	};

	// Small fixed ring rather than a growing map: it avoids hooking DeleteObject
	// (and the lifetime questions that brings) and the game only keeps a handful
	// of surfaces alive. A stale entry is harmless -- lookups match on HBITMAP.
	constexpr int kMaxDibs = 32;
	Dib   g_dibs[kMaxDibs];
	std::atomic<int> g_dibNext{ 0 };
	std::mutex g_dibMx;

	std::atomic<bool> g_want{ false };
	// Last observed signed heights of the presenting blit -- diagnostics, so the
	// orientation decision is inspectable instead of a silent guess.
	std::atomic<int>  g_blitHDst{ 0 };
	std::atomic<int>  g_blitHSrc{ 0 };
	// Signed height of the surface the capture actually read (GetObject at blit
	// time), which is NOT necessarily one of the surfaces in the creation
	// registry -- that distinction is the whole reason this is reported.
	std::atomic<int>  g_srcSignedH{ 0 };
	std::atomic<int>  g_usedTopDown{ -1 };
	std::atomic<bool> g_ready{ false };
	std::atomic<unsigned long> g_served{ 0 };

	std::mutex g_mx;
	std::vector<unsigned char> g_px;   // RGBA, top-down, ready to encode
	int g_w = 0, g_h = 0, g_srcBpp = 0;

	void CapLog(const char* m)
	{
		FILE* f = nullptr;
		if (fopen_s(&f, "C:\\Users\\benam\\source\\cpp\\D2MOO\\conformance\\behavioral\\overlay_gl_log.txt", "a") == 0 && f)
		{
			fprintf(f, "[vcapture] %s\n", m);
			fclose(f);
		}
	}

	const Dib* FindDib(HBITMAP b)
	{
		if (!b)
			return nullptr;
		for (int i = 0; i < kMaxDibs; ++i)
			if (g_dibs[i].bmp == b)
				return &g_dibs[i];
		return nullptr;
	}

	// Convert the DIB's rows into top-down RGBA. DIBs are BGR(A) and, unless the
	// height was supplied negative, BOTTOM-UP -- getting either wrong yields a
	// colour-swapped or vertically mirrored image that still "looks like" a
	// capture, which is the failure mode most likely to go unnoticed.
	// The pixel loop is split from the allocation so the SEH guard at the call
	// site wraps only raw memory reads: MSVC rejects __try around code that
	// needs object unwinding (C2712), and vector::resize does.
	void CopyRows(const Dib& d, unsigned char* dst0)
	{
		const int w = d.w;
		const int h = d.h < 0 ? -d.h : d.h;
		// Two independent flips compose: the DIB's storage order, and the flip the
		// present blit applies on its way to the screen. XOR, so double-flip is a
		// no-op rather than a mirrored frame.
		const bool topDown = (d.h < 0) != d.blitFlips;
		const int srcBytes = d.bpp / 8;
		const size_t srcStride = ((size_t)w * srcBytes + 3u) & ~(size_t)3u;  // DWORD-aligned
		const unsigned char* base = (const unsigned char*)d.bits;

		for (int y = 0; y < h; ++y)
		{
			const int srcRow = topDown ? y : (h - 1 - y);
			const unsigned char* s = base + (size_t)srcRow * srcStride;
			unsigned char* dst = dst0 + (size_t)y * w * 4u;
			if (d.bpp == 8)
			{
				// Palette INDICES, not colour. RGBQUAD is stored B,G,R,reserved.
				for (int x = 0; x < w; ++x, ++s, dst += 4)
				{
					const RGBQUAD& c = d.pal[*s];
					dst[0] = c.rgbRed;
					dst[1] = c.rgbGreen;
					dst[2] = c.rgbBlue;
					dst[3] = 255;
				}
			}
			else
			{
				for (int x = 0; x < w; ++x, s += srcBytes, dst += 4)
				{
					dst[0] = s[2];      // B,G,R -> R,G,B
					dst[1] = s[1];
					dst[2] = s[0];
					dst[3] = 255;       // DIB alpha is meaningless; force opaque
				}
			}
		}
	}

	// __try must live in a function that needs no object unwinding (C2712), so
	// the guard is its own frame: no locks, no vectors, just the raw reads.
	bool SafeCopyRows(const Dib& d, unsigned char* dst)
	{
		__try
		{
			CopyRows(d, dst);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	bool DibSupported(const Dib& d)
	{
		const int h = d.h < 0 ? -d.h : d.h;
		if (!d.bits || d.w <= 0 || h <= 0)
			return false;
		if (d.bpp == 8)
			return d.havePal;      // indices are meaningless without the table
		return d.bpp == 32 || d.bpp == 24;
	}
}

// ---------------------------------------------------------------- hooks in ----

// Called from the CreateDIBSection hook (D2Debugger.probe.cpp owns the detour).
extern "C" void D2Capture_NoteDib(void* bmp, void* bits, int w, int h, int bpp)
{
	if (!bmp || !bits)
		return;
	std::lock_guard<std::mutex> lk(g_dibMx);
	const int i = g_dibNext.fetch_add(1, std::memory_order_relaxed) % kMaxDibs;
	g_dibs[i].bmp = (HBITMAP)bmp;
	g_dibs[i].bits = bits;
	g_dibs[i].w = w;
	g_dibs[i].h = h;
	g_dibs[i].bpp = bpp;
}

// Called from the StretchBlt hook, BEFORE the blit runs -- the source DIB holds
// the finished frame at that moment. No-op unless a capture was requested, so
// the normal 49-frames-a-second path costs one predictable branch.
extern "C" void D2Capture_OnStretchBlt(void* hdcSrc, int hDst, int hSrc)
{
	if (!g_want.load(std::memory_order_acquire))
		return;

	// Ask GDI for the CURRENT descriptor rather than trusting our own registry.
	//
	// The first version looked the HBITMAP up in the CreateDIBSection ring and
	// used the recorded bits pointer -- and faulted, because HBITMAP VALUES ARE
	// REUSED after a bitmap is deleted. A stale entry therefore matched a live
	// handle and handed back freed memory. GetObject(DIBSECTION) closes that
	// hole completely: the pointer and dimensions come from the handle that is
	// selected into the DC right now, so a stale entry cannot be consulted at
	// all. The CreateDIBSection hook is now only informational (the probe's
	// surface list).
	HBITMAP b = (HBITMAP)GetCurrentObject((HDC)hdcSrc, OBJ_BITMAP);
	if (!b)
		return;
	DIBSECTION ds{};
	if (GetObject(b, sizeof(ds), &ds) != sizeof(ds) || !ds.dsBm.bmBits)
		return;                  // not a DIB section: leave the request pending

	Dib d;
	d.bmp = b;
	d.bits = ds.dsBm.bmBits;
	d.w = ds.dsBmih.biWidth;
	d.h = ds.dsBmih.biHeight;
	d.bpp = ds.dsBmih.biBitCount;

	// GetObject DOES NOT preserve the top-down flag. Measured: a surface the
	// CreateDIBSection hook recorded as -600 comes back from GetObject as +600,
	// so trusting this sign read a top-down surface bottom-up and produced a
	// perfectly plausible, perfectly upside-down frame.
	//
	// Recover the true orientation from the creation registry, matching on the
	// BITS POINTER (plus geometry) rather than the HBITMAP -- handle values are
	// reused after deletion, which is what faulted the first implementation.
	// Only the SIGN is taken from the registry; the pointer is compared, never
	// dereferenced, so a stale entry can at worst fail to match.
	{
		std::lock_guard<std::mutex> lk(g_dibMx);
		const int absH = d.h < 0 ? -d.h : d.h;
		for (int i = 0; i < kMaxDibs; ++i)
		{
			const auto& r = g_dibs[i];
			if (r.bits == d.bits && r.w == d.w && r.bpp == d.bpp &&
			    (r.h < 0 ? -r.h : r.h) == absH)
			{
				d.h = r.h;           // authoritative sign, as created
				break;
			}
		}
	}

	d.blitFlips = ((hDst < 0) != (hSrc < 0));
	g_srcSignedH.store(d.h, std::memory_order_relaxed);
	g_usedTopDown.store(((d.h < 0) != d.blitFlips) ? 1 : 0, std::memory_order_relaxed);
	g_blitHDst.store(hDst, std::memory_order_relaxed);
	g_blitHSrc.store(hSrc, std::memory_order_relaxed);

	// D2 renders 8bpp palettized (measured: every surface is 8bpp, including the
	// 1068x600 frame). The colour table lives on the DC the DIB is selected into,
	// which is exactly the source DC of this blit.
	if (d.bpp == 8)
		d.havePal = (GetDIBColorTable((HDC)hdcSrc, 0, 256, d.pal) > 0);

	g_want.store(false, std::memory_order_relaxed);

	bool ok = false;
	{
		std::lock_guard<std::mutex> lk(g_mx);
		// SEH insurance. We are on the game's render thread inside its present
		// path; a surface torn down between GetObject and the read would
		// otherwise take the whole process down. Same discipline the oracle
		// applies to every call it makes into game memory.
		if (DibSupported(d))
		{
			const int hh = d.h < 0 ? -d.h : d.h;
			g_px.resize((size_t)d.w * hh * 4u);
			ok = SafeCopyRows(d, g_px.data());
		}
		g_w = ok ? d.w : 0;
		g_h = ok ? (d.h < 0 ? -d.h : d.h) : 0;
		g_srcBpp = d.bpp;
	}
	if (ok)
		g_served.fetch_add(1, std::memory_order_relaxed);
	g_ready.store(true, std::memory_order_release);
}

// Orientation diagnostics for the last capture. Reported by /capture/frame so a
// mirrored frame can be diagnosed from the response instead of guessed at.
extern "C" void D2Capture_LastGeometry(int* srcSignedH, int* hDst, int* hSrc, int* usedTopDown)
{
	if (srcSignedH) *srcSignedH = g_srcSignedH.load(std::memory_order_relaxed);
	if (hDst)       *hDst       = g_blitHDst.load(std::memory_order_relaxed);
	if (hSrc)       *hSrc       = g_blitHSrc.load(std::memory_order_relaxed);
	if (usedTopDown)*usedTopDown= g_usedTopDown.load(std::memory_order_relaxed);
}

extern "C" unsigned long D2Capture_FrameCount()
{
	return g_served.load(std::memory_order_relaxed);
}

// ---------------------------------------------------------------- public ----

// Request a frame and write it as PNG. Blocks the CALLING (HTTP) thread until a
// frame is presented, or `timeoutMs` elapses.
//
// Returns 1 ok, -1 timed out (game not presenting?), -2 unsupported pixel
// format, -3 PNG write failed.
extern "C" int D2Capture_WriteFramePng(const char* path, int /*withOverlay*/,
                                       int timeoutMs, int* outW, int* outH)
{
	if (!path || !*path)
		return -3;

	g_ready.store(false, std::memory_order_relaxed);
	g_want.store(true, std::memory_order_release);

	const DWORD deadline = GetTickCount() + (DWORD)(timeoutMs > 0 ? timeoutMs : 3000);
	while (!g_ready.load(std::memory_order_acquire))
	{
		if (GetTickCount() > deadline)
		{
			g_want.store(false, std::memory_order_relaxed);
			return -1;
		}
		Sleep(5);
	}

	std::vector<unsigned char> px;
	int w = 0, h = 0, bpp = 0;
	{
		std::lock_guard<std::mutex> lk(g_mx);
		w = g_w; h = g_h; bpp = g_srcBpp;
		px = g_px;
	}
	if (outW) *outW = w;
	if (outH) *outH = h;
	if (w <= 0 || h <= 0 || px.empty())
	{
		char b[128];
		_snprintf_s(b, sizeof(b), _TRUNCATE, "unsupported DIB format: %d bpp", bpp);
		CapLog(b);
		return -2;
	}
	return stbi_write_png(path, w, h, 4, px.data(), w * 4) ? 1 : -3;
}

// Diagnostics for GET /capture/probe: what surfaces we know about, so an
// unsupported pixel format is visible rather than showing up as a broken image.
extern "C" int D2Capture_DibReport(char* buf, int cch)
{
	std::lock_guard<std::mutex> lk(g_dibMx);
	int n = _snprintf_s(buf, cch, _TRUNCATE, "\"dibs\":[");
	int emitted = 0;
	for (int i = 0; i < kMaxDibs; ++i)
	{
		if (!g_dibs[i].bmp)
			continue;
		n += _snprintf_s(buf + n, cch - n, _TRUNCATE,
			"%s{\"w\":%d,\"h\":%d,\"bpp\":%d,\"topDown\":%s}",
			emitted++ ? "," : "", g_dibs[i].w, g_dibs[i].h, g_dibs[i].bpp,
			g_dibs[i].h < 0 ? "true" : "false");
		if (n >= cch - 96)
			break;
	}
	_snprintf_s(buf + n, cch - n, _TRUNCATE, "]");
	return 1;
}
