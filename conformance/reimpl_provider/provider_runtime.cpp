// provider_runtime.cpp -- injected resolver storage (see provider_runtime.h).
#include "provider_runtime.h"
#include <cstring>
#include <Windows.h>   // GetModuleHandleA -- runtime module base, see D2MOO_Resolve

// Supplementary name->address table generated from pending_globals.json. The
// patch's baked table only changes on a game RESTART (it is compiled into
// D2Common.dll and the game runs elevated); this one ships with the provider,
// which D2Debugger hot-reloads on every prove. Regenerate with
// conformance/tools/gen_provider_globals.py.
#include "provider_globals.gen.h"

namespace {
	typedef void* (__cdecl* ResolveFn)(const char*);
	ResolveFn g_resolve = nullptr;
}

extern "C" {

// D2MOO_REIMPL_EXPORT: D2MOO_Provider_Init
// Injected by D2Debugger after MemoryLoadLibraryEx, passing D2MOO_ResolveGameFn.
void __cdecl D2MOO_Provider_Init(void* resolveGameFn)
{
	g_resolve = (ResolveFn)resolveGameFn;
}

// Verified NAME -> live game address. Null if unresolved / not injected.
//
// The injected resolver is asked FIRST and always wins: it carries the
// corrected_maps function names and the curated DATA_GLOBALS, which are the
// verified source of truth. Only when it has never heard of `name` do we fall
// back to the generated table -- so a freshly-wired global resolves after a
// provider hot-reload, without waiting for a patch rebuild + game restart, and
// a stale generated entry can never shadow a verified address.
void* __cdecl D2MOO_Resolve(const char* name)
{
	if (!name) return nullptr;
	if (g_resolve)
	{
		if (void* p = g_resolve(name)) return p;
	}
	// Binary search -- the generator emits strcmp order. A miss here is on the
	// hot path (shadow dispatchers resolve per call and only reach this table
	// for names the baked patch has never seen), so the whole-program wiring
	// must not cost a linear scan of every global in the DLL.
	int lo = 0, hi = g_d2moo_provider_global_count - 1;
	while (lo <= hi)
	{
		const int mid = lo + (hi - lo) / 2;
		const char* mname = g_d2moo_provider_globals[mid].name;
		if (!mname) break;
		const int c = strcmp(name, mname);
		if (c == 0)
		{
			// RUNTIME base + RVA (2026-07-30). Entries used to be Ghidra ABSOLUTE
			// addresses, which is only correct for a module that got its preferred
			// base. D2Client does not (0x03600000 live vs 0x6fab0000 in Ghidra), so
			// all 3,392 D2Client entries handed reimpls a pointer into unmapped
			// memory -- and a reimpl dereferencing it faults indistinguishably from
			// a wrong ABI.
			const uintptr_t base =
				(uintptr_t)GetModuleHandleA(g_d2moo_provider_globals[mid].module);
			if (!base) return nullptr;
			return (void*)(base + g_d2moo_provider_globals[mid].rva);
		}
		if (c < 0) hi = mid - 1;
		else       lo = mid + 1;
	}
	return nullptr;
}

}
