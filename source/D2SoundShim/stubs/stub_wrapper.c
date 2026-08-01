/* stub_wrapper.c -- stand-ins for ddraw.dll (cnc-ddraw) and glide3x.dll.
 *
 * Once the three D2 video backends are stubs, NOTHING imports these two -- but
 * SGD2FreeRes still LoadLibraryW's them by name (they are in its module list;
 * measured: removing just these two is the same fatal 0x7e dialog). So they
 * must exist and load; they never need to work.
 *
 * glide3x: SGD2FreeRes's wrapper patches are version-specific (its RTTI names
 * literally carry NGlide_3_10_0_658 / Sven_1_4_4_21), and against the CURRENT
 * real glide3x it already applies ZERO patches -- unknown wrapper, graceful
 * skip. A stub with no version resource is just another unknown wrapper. Pad
 * .text anyway, in case a future version patches blind.
 *
 * ddraw: no sgd2fr patch namespace targets it (verified in the RTTI dump), so
 * it only needs to load. Minimal creation exports are provided returning
 * "no hardware", purely so any stray GetProcAddress caller gets an honest
 * failure instead of a missing-export surprise.
 */
#include <windows.h>

#pragma section(".text")
__declspec(allocate(".text")) static const unsigned char g_patchPad[0x20000] = { 1 };

#define DDERR_NODIRECTDRAWHW 0x8876024AL

HRESULT WINAPI DirectDrawCreate(void* guid, void** out, void* outer)
{
    (void)guid; (void)outer;
    if (out) *out = (void*)0;
    return DDERR_NODIRECTDRAWHW;
}

HRESULT WINAPI DirectDrawCreateEx(void* guid, void** out, void* iid, void* outer)
{
    (void)guid; (void)iid; (void)outer;
    if (out) *out = (void*)0;
    return DDERR_NODIRECTDRAWHW;
}

HRESULT WINAPI DirectDrawEnumerateA(void* cb, void* ctx)
{
    (void)cb; (void)ctx;
    return 0; /* DD_OK, zero devices */
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved)
{
    (void)h; (void)reason; (void)reserved;
    /* Keep the pad alive against link-time folding. */
    return g_patchPad[0] != 0;
}
