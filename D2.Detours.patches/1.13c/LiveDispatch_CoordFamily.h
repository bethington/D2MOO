#pragma once
// LiveDispatch_CoordFamily.h -- Phase 2 of ../../conformance/LIVE_DISPATCH_FRAMEWORK_PLAN.md.
//
// Generalizes the Phase 1 hand-written dispatcher (proven live against real
// PD2-S12, all three modes, 2026-07-06) from ONE function
// (DUNGEON_GameTileToClientCoords) to all 5 coord-family functions -- they
// all share the proven `void __stdcall(int*, int*)` shape (see
// conformance/behavioral/README.md), so ONE macro instantiates a distinct
// dispatcher (atomic mode + trampoline slot + Thunk) per function -- the
// "Option A templated thunk" pattern from the plan, done via macro instead of
// a template because this project's MSVC config doesn't opt into C++17 (see
// the Phase 1 postmortem: C++17 inline variables failed to compile here).
//
// Verified offsets (D2Common+off, off = GhidraVA - 0x6FD50000), same ones
// proven in the Phase 0 Frida spike (pd2_phase0_hook_spike.js) and
// pd2_frida_agent.js -- resolved by VERIFIED OFFSET, never the scrambled
// .def ordinals (conformance/ORDINAL_RECONCILIATION.md):
//   DUNGEON_GameTileToClientCoords     0x4dac0
//   DUNGEON_GameTileToSubtileCoords    0x4d8a0
//   DUNGEON_ClientToGameCoords         0x4d8c0
//   DUNGEON_GameToClientCoords         0x4db40
//   DUNGEON_GameSubtileToClientCoords  0x4db70
//
// New in Phase 2 vs the Phase 1 spike:
// - covers all 5 functions, not just one.
// - per-thread REENTRANCY GUARD (LiveDispatch::tl_inDispatch, shared across
//   all 5 dispatchers): if a Reimpl call transitively re-enters ANY hooked
//   dispatcher on the same thread, that nested call forces Original mode --
//   never nests a Shadow comparison inside another. Required by the plan's
//   snapshotting section.
// - Shadow divergences are appended as self-contained, vectors/*.json-SCHEMA
//   JSON objects (one per line -- newline-delimited so concurrent/frequent
//   appends stay trivially safe; join with commas and wrap in `[...]` to get
//   a proper vectors/*.json array) to live_shadow_divergences.jsonl. A live
//   divergence becomes a permanent offline regression case for free -- see
//   conformance/d2moo_conform.cpp's coord-family dispatch cases and
//   conformance/vectors/coord_family.json, which round-trip this exact schema
//   through the REAL compiled D2MOO functions, no game process needed.
//
// NOTE: plain namespace-scope statics (not C++17 `inline` class members) --
// this header is only ever included from ONE translation unit
// (D2Common.patch.cpp), so ordinary statics are sufficient; no ODR risk.
// NOTE: Game.exe is a GUI-subsystem process with no console -- heartbeats and
// divergences go to file (readable without a debugger) and OutputDebugStringA
// (DebugView), matching the rest of D2.Detours' own logging convention.
#include <D2Dungeon.h>

#include <Windows.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>

// WS-1.5: NAME -> VERIFIED ADDRESS table (corrected_maps), so reimpls resolve
// game dependencies by verified address, never the scrambled export table.
// Regenerate with conformance/tools/gen_resolve_table.py.
#include "D2Common_ResolveTable.gen.h"

namespace LiveDispatch
{
	enum class Mode : int32_t { Original = 0, Reimpl = 1, Shadow = 2 };

	// Shared reentrancy guard across ALL coord dispatchers on this thread.
	static thread_local bool tl_inDispatch = false;

	// WS-1 hot-reload drain counter (GRADUATED_CONFORMANCE_PIPELINE_PLAN.md
	// "Design detail A"): incremented UNCONDITIONALLY around every reimpl call so
	// a hot-reload can quiesce (set all Original) then wait for this to reach 0
	// before FreeLibrary'ing the provider DLL -- never unmap code mid-call.
	static std::atomic<int> g_reimplInFlight{ 0 };

	// The coord family's shared reimpl signature.
	using ReimplFn = void(__stdcall*)(int*, int*);

	// SPIKE-ONLY hardcoded absolute paths (Phase 4's D2Debugger panel replaces
	// the env-var mode control + file logging here with a live UI + registry).
	static const char* kHeartbeatLogPath =
		"C:\\Users\\benam\\source\\cpp\\D2MOO\\conformance\\behavioral\\phase1_dispatch_log.txt";
	static const char* kDivergenceVectorsPath =
		"C:\\Users\\benam\\source\\cpp\\D2MOO\\conformance\\behavioral\\live_shadow_divergences.jsonl";
}

// Generates one dispatcher namespace NS for coord function FN (D2MOO's own
// symbol, called directly for both the "real" identity in logs/vectors and
// the actual Reimpl invocation -- this family has no separate reimpl name).
#define D2MOO_DEFINE_COORD_DISPATCH(NS, FN)                                                     \
namespace NS {                                                                                   \
	using LiveDispatch::Mode;                                                                    \
	static std::atomic<int32_t> mode{ (int32_t)Mode::Original };                                 \
	static void* trampoline = nullptr;                                                           \
	static uint64_t hits = 0, divergences = 0;                                                   \
	/* WS-1: the Reimpl/Shadow call goes through this SWAPPABLE pointer (default  */             \
	/* = D2MOO's own FN). A hot-reloadable provider DLL repoints it via           */             \
	/* D2MOO_LiveDispatch_SetReimpl -- add/replace an equivalent with no restart. */             \
	static LiveDispatch::ReimplFn reimpl = &FN;                                                  \
                                                                                                  \
	static void WriteHeartbeat()                                                                 \
	{                                                                                             \
		FILE* f = nullptr;                                                                       \
		if (fopen_s(&f, LiveDispatch::kHeartbeatLogPath, "a") == 0 && f)                         \
		{                                                                                         \
			fprintf(f, "[heartbeat] fn=" #FN " mode=%d hits=%llu divergences=%llu trampoline=%p\n", \
				mode.load(), (unsigned long long)hits, (unsigned long long)divergences, trampoline); \
			fclose(f);                                                                           \
		}                                                                                         \
	}                                                                                             \
	static void WriteDivergence(int inX, int inY, int origX, int origY, int rX, int rY)           \
	{                                                                                             \
		FILE* f = nullptr;                                                                       \
		if (fopen_s(&f, LiveDispatch::kDivergenceVectorsPath, "a") == 0 && f)                     \
		{                                                                                         \
			fprintf(f,                                                                           \
				"{\"fn\":\"" #FN "\",\"in\":{\"x\":%d,\"y\":%d},"                                \
				"\"out\":{\"x\":%d,\"y\":%d},\"reimpl_out\":{\"x\":%d,\"y\":%d},"                \
				"\"note\":\"live SHADOW divergence captured against real PD2-S12\"}\n",          \
				inX, inY, origX, origY, rX, rY);                                                 \
			fclose(f);                                                                           \
		}                                                                                         \
	}                                                                                             \
	static void __stdcall Thunk(int* pX, int* pY)                                                \
	{                                                                                             \
		++hits;                                                                                  \
		static DWORD lastSnapshotTick = 0;                                                       \
		const DWORD now = GetTickCount();                                                        \
		if (hits == 1 || now - lastSnapshotTick > 2000) { lastSnapshotTick = now; WriteHeartbeat(); } \
                                                                                                  \
		using Fn = void(__stdcall*)(int*, int*);                                                 \
		const Fn orig = (Fn)trampoline;                                                          \
		/* Reentrancy guard: a nested re-entry into ANY coord dispatcher on   */                 \
		/* this thread (e.g. FN calling another hooked coord fn internally)  */                  \
		/* forces Original, never nests a Shadow comparison.                 */                  \
		const Mode m = LiveDispatch::tl_inDispatch ? Mode::Original                              \
			: (Mode)mode.load(std::memory_order_relaxed);                                        \
                                                                                                  \
		const LiveDispatch::ReimplFn rfn = reimpl;                                                \
		if (m == Mode::Reimpl && rfn)                                                            \
		{                                                                                         \
			LiveDispatch::tl_inDispatch = true;                                                   \
			++LiveDispatch::g_reimplInFlight;   /* drain guard (unconditional on reimpl path) */  \
			rfn(pX, pY);                                                                          \
			--LiveDispatch::g_reimplInFlight;                                                     \
			LiveDispatch::tl_inDispatch = false;                                                  \
			return;                                                                               \
		}                                                                                         \
		if (m != Mode::Shadow || !orig || !rfn) { if (orig) orig(pX, pY); return; }               \
                                                                                                  \
		/* Shadow: snapshot inputs, let ORIGINAL run and win (game observes  */                  \
		/* only its result), then run reimpl on an independent copy.        */                   \
		const int inX = *pX, inY = *pY;                                                          \
		orig(pX, pY);                                                                            \
		const int origX = *pX, origY = *pY;                                                      \
                                                                                                  \
		int rX = inX, rY = inY;                                                                  \
		LiveDispatch::tl_inDispatch = true;                                                       \
		++LiveDispatch::g_reimplInFlight;                                                         \
		rfn(&rX, &rY);                                                                            \
		--LiveDispatch::g_reimplInFlight;                                                         \
		LiveDispatch::tl_inDispatch = false;                                                      \
                                                                                                  \
		if (rX != origX || rY != origY)                                                          \
		{                                                                                         \
			++divergences;                                                                       \
			char buf[256];                                                                       \
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,                                             \
				"[LiveDispatch] SHADOW DIVERGENCE " #FN ": in=(%d,%d) orig=(%d,%d) reimpl=(%d,%d)\n", \
				inX, inY, origX, origY, rX, rY);                                                  \
			OutputDebugStringA(buf);                                                              \
			WriteDivergence(inX, inY, origX, origY, rX, rY);                                      \
		}                                                                                         \
		*pX = origX; *pY = origY;   /* game must observe the ORIGINAL's result */                \
	}                                                                                             \
                                                                                                  \
	static bool InitModeFromEnv()                                                                \
	{                                                                                             \
		char buf[16] = {};                                                                       \
		if (GetEnvironmentVariableA("D2MOO_DISPATCH_MODE", buf, sizeof(buf)))                    \
		{                                                                                         \
			Mode m2 = Mode::Original;                                                            \
			if (_stricmp(buf, "reimpl") == 0) m2 = Mode::Reimpl;                                  \
			else if (_stricmp(buf, "shadow") == 0) m2 = Mode::Shadow;                             \
			mode.store((int32_t)m2);                                                              \
		}                                                                                         \
		WriteHeartbeat();                                                                        \
		return true;                                                                              \
	}                                                                                             \
	static const bool g_modeInit = InitModeFromEnv();                                             \
}

D2MOO_DEFINE_COORD_DISPATCH(GameTileToClientCoordsDispatch, DUNGEON_GameTileToClientCoords)
D2MOO_DEFINE_COORD_DISPATCH(GameTileToSubtileCoordsDispatch, DUNGEON_GameTileToSubtileCoords)
D2MOO_DEFINE_COORD_DISPATCH(ClientToGameCoordsDispatch, DUNGEON_ClientToGameCoords)
D2MOO_DEFINE_COORD_DISPATCH(GameToClientCoordsDispatch, DUNGEON_GameToClientCoords)
D2MOO_DEFINE_COORD_DISPATCH(GameSubtileToClientCoordsDispatch, DUNGEON_GameSubtileToClientCoords)

// --- Live-dispatch bridge (Phase 4, conformance/LIVE_DISPATCH_FRAMEWORK_PLAN.md).
// C exports so an EXTERNAL module (D2Debugger) can read/toggle these dispatchers
// live. Required because D2Debugger's normal linking would bind D2Common symbols
// to the REAL game D2Common.dll (whose ordinals are scrambled -- see
// ORDINAL_RECONCILIATION.md), NOT this patch copy where the atomics live.
// D2Debugger instead enumerates loaded modules, finds the ONE exporting these
// (the real PD2 D2Common doesn't), and GetProcAddress'es them. Each exported
// function runs IN this patch module and touches its OWN atomics -- no direct
// cross-module memory access.
namespace LiveDispatchBridge
{
	struct Entry
	{
		const char* name;
		uint32_t offset; // D2Common offset -- lets the UI match a dispatcher to its
		                 // profiler function DATA-DRIVEN (no hardcoded list), so new
		                 // equivalents auto-appear as shadowable when added here.
		std::atomic<int32_t>* mode;
		uint64_t* hits;
		uint64_t* divergences;
		void** reimplSlot;     // address of the dispatcher's swappable reimpl pointer (WS-1)
		void** trampolineSlot; // address of the Detours trampoline (raw original) -- WS-5 oracle
	};
	static Entry g_table[] = {
		{ "DUNGEON_GameTileToClientCoords",    0x4dac0, &GameTileToClientCoordsDispatch::mode,    &GameTileToClientCoordsDispatch::hits,    &GameTileToClientCoordsDispatch::divergences,    (void**)&GameTileToClientCoordsDispatch::reimpl,    &GameTileToClientCoordsDispatch::trampoline },
		{ "DUNGEON_GameTileToSubtileCoords",   0x4d8a0, &GameTileToSubtileCoordsDispatch::mode,   &GameTileToSubtileCoordsDispatch::hits,   &GameTileToSubtileCoordsDispatch::divergences,   (void**)&GameTileToSubtileCoordsDispatch::reimpl,   &GameTileToSubtileCoordsDispatch::trampoline },
		{ "DUNGEON_ClientToGameCoords",        0x4d8c0, &ClientToGameCoordsDispatch::mode,        &ClientToGameCoordsDispatch::hits,        &ClientToGameCoordsDispatch::divergences,        (void**)&ClientToGameCoordsDispatch::reimpl,        &ClientToGameCoordsDispatch::trampoline },
		{ "DUNGEON_GameToClientCoords",        0x4db40, &GameToClientCoordsDispatch::mode,        &GameToClientCoordsDispatch::hits,        &GameToClientCoordsDispatch::divergences,        (void**)&GameToClientCoordsDispatch::reimpl,        &GameToClientCoordsDispatch::trampoline },
		{ "DUNGEON_GameSubtileToClientCoords", 0x4db70, &GameSubtileToClientCoordsDispatch::mode, &GameSubtileToClientCoordsDispatch::hits, &GameSubtileToClientCoordsDispatch::divergences, (void**)&GameSubtileToClientCoordsDispatch::reimpl, &GameSubtileToClientCoordsDispatch::trampoline },
	};
	static const int kCount = (int)(sizeof(g_table) / sizeof(g_table[0]));
}

extern "C" {
	__declspec(dllexport) int __cdecl D2MOO_LiveDispatch_GetCount()
	{
		return LiveDispatchBridge::kCount;
	}
	__declspec(dllexport) const char* __cdecl D2MOO_LiveDispatch_GetName(int i)
	{
		return (i >= 0 && i < LiveDispatchBridge::kCount) ? LiveDispatchBridge::g_table[i].name : "";
	}
	// D2Common offset of dispatcher i -- the UI matches this to a profiler
	// function so newly-added equivalents auto-appear with a mode toggle.
	__declspec(dllexport) unsigned int __cdecl D2MOO_LiveDispatch_GetOffset(int i)
	{
		return (i >= 0 && i < LiveDispatchBridge::kCount) ? LiveDispatchBridge::g_table[i].offset : 0xFFFFFFFFu;
	}
	__declspec(dllexport) int __cdecl D2MOO_LiveDispatch_GetMode(int i)
	{
		return (i >= 0 && i < LiveDispatchBridge::kCount)
			? LiveDispatchBridge::g_table[i].mode->load(std::memory_order_relaxed) : 0;
	}
	__declspec(dllexport) void __cdecl D2MOO_LiveDispatch_SetMode(int i, int m)
	{
		if (i >= 0 && i < LiveDispatchBridge::kCount)
			LiveDispatchBridge::g_table[i].mode->store(m, std::memory_order_relaxed);
	}
	__declspec(dllexport) unsigned long long __cdecl D2MOO_LiveDispatch_GetHits(int i)
	{
		return (i >= 0 && i < LiveDispatchBridge::kCount) ? *LiveDispatchBridge::g_table[i].hits : 0ull;
	}
	__declspec(dllexport) unsigned long long __cdecl D2MOO_LiveDispatch_GetDivergences(int i)
	{
		return (i >= 0 && i < LiveDispatchBridge::kCount) ? *LiveDispatchBridge::g_table[i].divergences : 0ull;
	}

	// --- WS-5 direct-call oracle support (GRADUATED_CONFORMANCE_PIPELINE_PLAN.md) ---
	// Expose the raw original (Detours trampoline) and the currently-bound reimpl
	// so an external oracle can call BOTH with chosen inputs and compare -- a
	// deterministic, mode-independent proof (vs. passively waiting for the game to
	// exercise a shadow path). GetTrampoline is null until the dispatcher is hooked.
	__declspec(dllexport) void* __cdecl D2MOO_LiveDispatch_GetTrampoline(int i)
	{
		return (i >= 0 && i < LiveDispatchBridge::kCount) ? *LiveDispatchBridge::g_table[i].trampolineSlot : nullptr;
	}
	__declspec(dllexport) void* __cdecl D2MOO_LiveDispatch_GetReimpl(int i)
	{
		return (i >= 0 && i < LiveDispatchBridge::kCount) ? *LiveDispatchBridge::g_table[i].reimplSlot : nullptr;
	}

	// --- WS-1.5 verified-address resolver (detail A2) ---
	// Resolve a D2Common function by NAME to its VERIFIED game address (bypassing
	// the scrambled export table). This is the foundation both the MemoryModule
	// custom import resolver and the dependency-injection fallback call. Returns
	// the absolute address (game D2Common at its preferred base 0x6fd50000; add
	// ASLR rebasing here if it ever loads elsewhere). Null if the name is unknown.
	__declspec(dllexport) void* __cdecl D2MOO_ResolveGameFn(const char* name)
	{
		if (!name) return nullptr;
		for (int i = 0; i < g_d2moo_resolve_count; ++i)
			if (strcmp(name, g_d2moo_resolve_table[i].name) == 0)
				return (void*)(uintptr_t)g_d2moo_resolve_table[i].addr;
		return nullptr;
	}

	// --- WS-1 hot-reload bridge (GRADUATED_CONFORMANCE_PIPELINE_PLAN.md detail A) ---
	// Repoint dispatcher i's reimpl to a function from the (re)loaded provider DLL.
	// Pass nullptr to fall back to the baked-in D2MOO default. Call ONLY between
	// QuiesceForReload() and re-enabling modes.
	__declspec(dllexport) void __cdecl D2MOO_LiveDispatch_SetReimpl(int i, void* fn)
	{
		if (i >= 0 && i < LiveDispatchBridge::kCount)
			*LiveDispatchBridge::g_table[i].reimplSlot = fn;
	}

	// Quiesce for a provider hot-reload: force every dispatcher to Original (no new
	// reimpl calls), then spin until in-flight reimpl calls drain. Returns 1 if
	// drained (safe to FreeLibrary the provider), 0 on timeout (do NOT unload).
	__declspec(dllexport) int __cdecl D2MOO_LiveDispatch_QuiesceForReload()
	{
		for (int i = 0; i < LiveDispatchBridge::kCount; ++i)
			LiveDispatchBridge::g_table[i].mode->store((int32_t)LiveDispatch::Mode::Original,
				std::memory_order_seq_cst);
		for (int spin = 0; spin < 5000; ++spin) // ~5s bound
		{
			if (LiveDispatch::g_reimplInFlight.load(std::memory_order_seq_cst) == 0)
				return 1;
			Sleep(1);
		}
		return LiveDispatch::g_reimplInFlight.load(std::memory_order_seq_cst) == 0 ? 1 : 0;
	}
}
