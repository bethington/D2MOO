// D2Client.patch.cpp -- single-TU home for D2Client's shadow-dispatch
// conformance machinery. Mirrors D2Common.patch.cpp's pattern, minus the
// legacy ordinal-patching boilerplate D2Common carries from its pre-
// DllPreLoadHook era: D2Client has no such history, and DetoursPatch.cpp
// treats DllPreLoadHook as fully independent of the legacy GetPatchAction
// exports (a patch DLL with ONLY DllPreLoadHook is a supported, complete
// config -- confirmed against external/D2.Detours/source/src/DetoursPatch.cpp).
//
// No coord family here (LiveDispatch_CoordFamily.h is D2Common-specific), so
// D2Client_ShadowDispatch.gen.h is generated STANDALONE: it owns its own
// D2MOO_LiveDispatch_* bridge exports (see gen_shadow_dispatch.py's
// `standalone` mode), rather than delegating from a hand-written coord header.
#include <DetoursPatch.h>

// Generated shadow dispatchers (conformance/shadow_manifest.D2Client.json).
// Included AFTER DetoursPatch.h (Install() uses HookContext/PatchAction).
#include "D2Client_ShadowDispatch.gen.h"

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wmicrosoft-cast"
#endif

#ifdef D2_VERSION_113C
extern "C" {
    __declspec(dllexport)
    uint32_t __cdecl DllPreLoadHook(HookContext* ctx, const wchar_t* dllName)
    {
        (void)dllName;   // only ever called for D2Client.dll (this patch's own target)
        LiveDispatchGen::Install(ctx);
        return 0;
    }
}

#include <type_traits>
static_assert(std::is_same<decltype(DllPreLoadHook)*, DllPreLoadHookType>::value,
             "Ensure calling convention doesn't change");
#endif
