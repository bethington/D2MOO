#pragma once
// provider_runtime.h -- data-import resolver via DEPENDENCY INJECTION.
//
// <cstdint> is included here so EVERY candidate can freely use the fixed-width
// integer types (uint32_t/uint8_t/int16_t/...). A drafted reimpl that reached
// for uint32_t used to fail to COMPILE (the header only had plain C types), and
// because all candidates compile into the ONE provider DLL, that one failure
// poisoned the whole build -> the prove reported live_prove_failed for a reimpl
// that was actually CORRECT (found 2026-07-08: GetUnitPathCoordY proved 8/8 by
// hand yet the pipeline batch failed it). Making the widths always available
// removes that entire failure class.
#include <cstdint>

// Ghidra-decompiler pseudotype shim. A drafted reimpl is generated FROM the
// decompiled listing, and the model routinely copies Ghidra's non-standard type
// spellings (uint/ushort/byte/undefined4/...) straight into the C++. None are
// standard types, so the candidate fails to COMPILE -- and because all candidates
// share one provider DLL, that poisons the whole build and the prover misreports
// it as live_prove_failed (see the compile-cascade note; 2026-07-08 the LLM's
// GetUnitPathCoordY used `uint` and failed the batch though the reimpl was correct
// and proved 8/8 by hand). Mapping the spellings here removes the class outright.
typedef uint8_t  undefined;
typedef uint8_t  undefined1;
typedef uint16_t undefined2;
typedef uint32_t undefined4;
typedef uint64_t undefined8;
typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef unsigned char  byte;
typedef unsigned int   uint3;   // Ghidra 3-byte int, widened (only used as a value carrier)

// Win32 integer spellings. The model also reaches for these (DWORD/WORD/BYTE/...)
// since the game is Windows code; windows.h is NOT included in the provider TU, so
// they must be provided here too (2026-07-08: the redraft used DWORD and cascaded).
// Defined EXACTLY as <windows.h> does, so a candidate that transitively includes it
// sees an identical (legal) redefinition, not a conflict.
typedef unsigned long  DWORD;
typedef unsigned short WORD;
typedef unsigned char  BYTE;
typedef int            BOOL;
typedef unsigned int   UINT;
typedef unsigned long  ULONG;
typedef void*          LPVOID;
typedef unsigned char* LPBYTE;
typedef unsigned long* LPDWORD;
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
