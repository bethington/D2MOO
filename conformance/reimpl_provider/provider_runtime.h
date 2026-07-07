#pragma once
// provider_runtime.h -- data-import resolver via DEPENDENCY INJECTION.
//
// A reimpl often needs to read REAL game state (a global data table, a global
// pointer). The provider is a standalone in-memory module, so it can't link
// those symbols. Instead D2Debugger injects the verified-address resolver
// (D2MOO_ResolveGameFn, WS-1.5) into the provider right after loading it, via
// the exported D2MOO_Provider_Init(resolve). Reimpls then call D2MOO_Resolve
// ("name") to get a game function/global address by its VERIFIED name -- never
// a hardcoded absolute address, never the scrambled export table.
//
// Names come from conformance/tools/gen_resolve_table.py (corrected_maps for
// functions + a curated DATA_GLOBALS list for data). See the graduated plan's
// "stateful rungs".

extern "C" {
	// Called by D2Debugger once per (re)load with D2MOO_ResolveGameFn.
	void __cdecl D2MOO_Provider_Init(void* resolveGameFn);
	// Reimpls call this: verified NAME -> live game address (null if unresolved
	// or the resolver wasn't injected).
	void* __cdecl D2MOO_Resolve(const char* name);
}
