// SGD2FreeRes.patch.cpp -- single-TU home for SGD2FreeRes's shadow-dispatch
// conformance machinery. Mirrors D2Client.patch.cpp exactly (standalone mode,
// DllPreLoadHook only, no legacy ordinal-patching boilerplate).
//
// FIRST NON-D2 PATCH TARGET. Everything above this file is D2's own DLLs, but
// nothing in D2.Detours is D2-specific: D2DetoursRegisterPatchFolder enumerates
// *.dll in the patch folder and registers each one by FILENAME, so a patch DLL
// named SGD2FreeRes.dll is applied to the module of that name when it loads.
// Only D2MOO's CMake side had a fixed list (D2_KNOWN_DLLS), which is a
// build-time convenience rather than a constraint.
//
// WHY THIS MODULE. SGD2FreeRes-GDI is built from source we control, and the
// deployed DLL is MD5-identical to our own Release build, so its documentation
// and its reimplementations can be GRADED against ground truth -- which is true
// of no other binary in the corpus. See conformance/shadow_manifest.SGD2FreeRes.json.
//
// KNOWN RISK, recorded rather than assumed away: D2.Detours patches by
// intercepting LoadLibrary, and SGD2FreeRes.dll is loaded by PD2's launcher. If
// it loads BEFORE the interception hook is installed, this patch never applies
// and the dispatcher silently never installs -- the same failure the D2Client
// manifest records for a CRT codepage-init call. The test is empirical and
// cheap: after deploy, /dispatchers either lists CLIENT_SetWorldView or it does
// not, and its hit counter either climbs or stays at zero.
//
// No coord family here (LiveDispatch_CoordFamily.h is D2Common-specific), so
// SGD2FreeRes_ShadowDispatch.gen.h is generated STANDALONE: it owns its own
// D2MOO_LiveDispatch_* bridge exports (gen_shadow_dispatch.py `standalone` mode).
#include <DetoursPatch.h>

// Generated shadow dispatchers (conformance/shadow_manifest.SGD2FreeRes.json).
// Included AFTER DetoursPatch.h (Install() uses HookContext/PatchAction).
#include "SGD2FreeRes_ShadowDispatch.gen.h"

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wmicrosoft-cast"
#endif

#ifdef D2_VERSION_113C
extern "C" {
    __declspec(dllexport)
    uint32_t __cdecl DllPreLoadHook(HookContext* ctx, const wchar_t* dllName)
    {
        (void)dllName;   // only ever called for SGD2FreeRes.dll (this patch's own target)
        LiveDispatchGen::Install(ctx);
        return 0;
    }
}

#include <type_traits>
static_assert(std::is_same<decltype(DllPreLoadHook)*, DllPreLoadHookType>::value,
             "Ensure calling convention doesn't change");
#endif
