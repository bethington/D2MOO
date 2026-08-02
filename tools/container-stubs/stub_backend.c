/* stub_backend.c -- a video backend that exists to be loaded, never used.
 *
 * WHY. SGD2FreeRes eagerly LoadLibraryW's every D2 video backend and PATCHES
 * them at version-specific hard-coded offsets -- measured live with GDI active:
 * D2DDraw +27 bytes, D2Direct3D +24, D2Glide +23, all in modules that are not
 * rendering anything. Removing the DLLs is a fatal ERROR_MOD_NOT_FOUND dialog;
 * a naive few-KB stub is worse -- the deepest patch site is rva 0x10040, far
 * past a small image, so the patcher would write unmapped memory and crash the
 * game at startup, every time.
 *
 * SO: pad .text well past every measured patch offset. The patches land in this
 * dead padding, and nothing ever executes it because D2gfx calls only into the
 * SELECTED renderer -- which is D2Gdi, the one real backend left.
 *
 * The interface is easy to satisfy: every D2 video backend exports exactly ONE
 * function, NONAME at ordinal 10000 (verified with dumpbin) -- the driver-table
 * entry D2gfx resolves. We export a function returning NULL at that ordinal; it
 * is only ever called if someone selects this renderer, which the container
 * build never does.
 *
 * Padding is 0x20000 bytes: double the deepest measured patch site, so a future
 * SGD2FreeRes with somewhat different offsets still lands inside the image.
 */
#include <windows.h>

/* Force a large .text: allocate the pad there explicitly. The first byte is
 * nonzero so the block is initialized data with real storage, not something a
 * linker can fold away. */
#pragma section(".text")
__declspec(allocate(".text")) static const unsigned char g_patchPad[0x20000] = { 1 };

/* The ordinal-10000 driver entry. Returning NULL tells any caller there is no
 * driver table here -- and no caller should ever ask. */
void* __stdcall D2StubDriverEntry(void)
{
    /* Reference the pad so even an aggressive whole-program optimizer cannot
     * argue it unused. Never true at runtime. */
    return g_patchPad[1] ? (void*)g_patchPad : (void*)0;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved)
{
    (void)h; (void)reason; (void)reserved;
    return TRUE;
}
