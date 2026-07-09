// D2Debugger.profiler.cpp -- Phase 1 of conformance/DYNAMIC_PROFILER_PLAN.md.
//
// Whole-engine dynamic profiler: mass count-only hooks on the REAL game DLL's
// functions, so we can read which functions PD2 actually executes and how often
// -- objective data for prioritizing conformance work.
//
// MECHANISM (the universal counting stub): for each target function we generate
// a 20-byte machine-code stub that is calling-convention-AGNOSTIC because it
// never touches arguments:
//     add dword [counter_lo], 1     ; 83 05 <addr32> 01
//     adc dword [counter_hi], 0     ; 83 15 <addr32> 00   (64-bit, no wrap)
//     jmp dword [trampoline_ptr]    ; FF 25 <addr32>      ; -> real function
// Detours redirects the target's prologue to this stub; the stub counts, then
// jmps to the Detours trampoline (saved prologue + jmp back), which runs the
// real function; its own ret returns to the original caller. One `inc`-class op
// of overhead per call. Counters live in THIS module's memory -- the UI reads
// them directly, no cross-DLL bridge needed (that's only for the Tier-2
// Original/Reimpl/Shadow dispatchers in the patch D2Common).
//
// Phase 1 scope: D2Common EXPORTS only (1,172, from
// conformance/profiler/D2Common.functions.tsv, built by
// tools/build_profiler_tables.py). Hooks are installed on demand (button), not
// at startup, and each DetourAttach failure is skipped gracefully -- so an
// un-hookable prologue never aborts the batch or crashes the game.
#include <imgui.h>
#include "D2Debugger.h"

#include <Windows.h>
#include <detours.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace
{
	struct ProfFunc
	{
		uint64_t prefAddr = 0;
		uint64_t prefBase = 0;
		int ordinal = -1;
		std::string category;
		std::string name;
		void* runtimeAddr = nullptr;
		bool hooked = false;
	};

	std::vector<ProfFunc> g_funcs;
	std::vector<uint64_t> g_counters;    // parallel to g_funcs; stub increments this
	std::vector<void*>    g_trampolines; // parallel; before attach = runtimeAddr, after = trampoline
	unsigned char* g_stubPage = nullptr;
	const int STUB_SIZE = 20;

	bool g_tableLoaded = false;
	bool g_installed = false;
	int  g_hookedCount = 0;
	int  g_skippedCount = 0;
	void* g_gameD2CommonBase = nullptr;

	// SPIKE-only hardcoded path (matches the rest of the framework's logging).
	const char* kTablePath =
		"C:\\Users\\benam\\source\\cpp\\D2MOO\\conformance\\profiler\\D2Common.functions.tsv";

	// The 5 coord offsets already owned by the Tier-2 dispatchers in patch
	// D2Common -- skip them here to avoid a double-hook (their counts show in the
	// "Live Dispatch Registry" panel via the bridge).
	bool IsDispatcherOwnedOffset(uint64_t off)
	{
		return off == 0x4dac0 || off == 0x4d8a0 || off == 0x4d8c0 || off == 0x4db40 || off == 0x4db70;
	}

	// The REAL game D2Common.dll (NOT D2MOO's patch copy). Both share the base
	// name; the patch is the one exporting D2MOO_LiveDispatch_GetCount, so the
	// real game one is the module that does NOT.
	void* FindGameD2CommonBase()
	{
		void* result = nullptr;
		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
		if (snap == INVALID_HANDLE_VALUE)
			return nullptr;
		MODULEENTRY32W me{};
		me.dwSize = sizeof(me);
		if (Module32FirstW(snap, &me))
		{
			do
			{
				if (_wcsicmp(me.szModule, L"D2Common.dll") == 0)
				{
					if (GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetCount") == nullptr)
					{
						result = me.hModule; // the real game copy
						break;
					}
					if (!result)
						result = me.hModule; // fallback: first D2Common seen
				}
			} while (Module32NextW(snap, &me));
		}
		CloseHandle(snap);
		return result;
	}

	void LoadTable()
	{
		if (g_tableLoaded)
			return;
		g_tableLoaded = true;

		FILE* f = nullptr;
		if (fopen_s(&f, kTablePath, "r") != 0 || !f)
			return;
		char line[512];
		bool header = true;
		while (fgets(line, sizeof(line), f))
		{
			if (header) { header = false; continue; }
			// pref_addr \t pref_base \t ordinal \t category \t name
			char* ctx = nullptr;
			char* a = strtok_s(line, "\t", &ctx);
			char* b = strtok_s(nullptr, "\t", &ctx);
			char* o = strtok_s(nullptr, "\t", &ctx);
			char* c = strtok_s(nullptr, "\t", &ctx);
			char* n = strtok_s(nullptr, "\t\r\n", &ctx);
			if (!a || !b || !o || !c || !n)
				continue;
			ProfFunc pf;
			pf.prefAddr = _strtoui64(a, nullptr, 16);
			pf.prefBase = _strtoui64(b, nullptr, 16);
			pf.ordinal = atoi(o);
			pf.category = c;
			pf.name = n;
			g_funcs.push_back(std::move(pf));
		}
		fclose(f);

		g_counters.assign(g_funcs.size(), 0ull);
		g_trampolines.assign(g_funcs.size(), nullptr);
	}

	void EmitStub(unsigned char* p, uint64_t* counter, void** tramp)
	{
		const uint32_t cLo = (uint32_t)(uintptr_t)counter;
		const uint32_t cHi = cLo + 4;
		const uint32_t pPtr = (uint32_t)(uintptr_t)tramp;
		// add dword [cLo], 1
		*p++ = 0x83; *p++ = 0x05; *(uint32_t*)p = cLo; p += 4; *p++ = 0x01;
		// adc dword [cHi], 0
		*p++ = 0x83; *p++ = 0x15; *(uint32_t*)p = cHi; p += 4; *p++ = 0x00;
		// jmp dword [pPtr]
		*p++ = 0xFF; *p++ = 0x25; *(uint32_t*)p = pPtr; p += 4;
	}

	// Addresses already hooked -- by us OR by the dispatchers. CRITICAL: D2Common
	// has ALIASED ordinals (multiple ordinals -> the SAME address; the D2.Detours
	// source documents e.g. @10089 and @10985 sharing an address). Hooking the
	// same address twice double-patches the prologue and crashes the game. This
	// set makes every address hook AT MOST once.
	std::set<void*> g_hookedAddrs;
	bool g_stubsReady = false;
	std::set<std::string> g_installedCats;

	// One-time: resolve the game module, compute runtime addresses, generate all
	// stubs. Cheap (~23 KB of stubs for 1,172 fns). Does NOT attach anything.
	bool EnsureStubs()
	{
		if (g_stubsReady)
			return true;
		LoadTable();
		if (g_funcs.empty())
			return false;
		g_gameD2CommonBase = FindGameD2CommonBase();
		if (!g_gameD2CommonBase)
			return false;

		const size_t n = g_funcs.size();
		g_stubPage = (unsigned char*)VirtualAlloc(nullptr, n * STUB_SIZE,
			MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!g_stubPage)
			return false;
		for (size_t i = 0; i < n; ++i)
		{
			const uint64_t off = g_funcs[i].prefAddr - g_funcs[i].prefBase;
			g_funcs[i].runtimeAddr = (void*)((uintptr_t)g_gameD2CommonBase + (uintptr_t)off);
			EmitStub(g_stubPage + i * STUB_SIZE, &g_counters[i], &g_trampolines[i]);
		}
		FlushInstructionCache(GetCurrentProcess(), g_stubPage, n * STUB_SIZE);
		g_stubsReady = true;
		return true;
	}

	// True only if addr is committed, executable memory (a real code function).
	// CRITICAL: the export table includes DATA exports (e.g. g_pDataTables, the
	// global data-tables root @ 0x6fde9e1c). Detours-"hooking" a data address
	// patches its bytes with a jump, corrupting the pointer -> the game
	// dereferences garbage on next access -> whole-process crash. This gate skips
	// every non-code export.
	bool IsExecutableAddr(void* addr)
	{
		MEMORY_BASIC_INFORMATION mbi{};
		if (VirtualQuery(addr, &mbi, sizeof(mbi)) == 0 || mbi.State != MEM_COMMIT)
			return false;
		const DWORD prot = mbi.Protect & 0xFF;
		return prot == PAGE_EXECUTE || prot == PAGE_EXECUTE_READ ||
			prot == PAGE_EXECUTE_READWRITE || prot == PAGE_EXECUTE_WRITECOPY;
	}

	// Attach a single function's hook. MUST be called inside a Detours
	// transaction. Skips dispatcher-owned, already-hooked, aliased-duplicate,
	// and NON-CODE (data export) addresses, plus any target Detours refuses.
	bool AttachOne(int i)
	{
		if (g_funcs[i].hooked)
			return false;
		const uint64_t off = g_funcs[i].prefAddr - g_funcs[i].prefBase;
		if (IsDispatcherOwnedOffset(off))
			return false;
		void* addr = g_funcs[i].runtimeAddr;
		if (g_hookedAddrs.count(addr))
			return false; // alias -> already hooked at this address
		if (!IsExecutableAddr(addr))
			return false; // data export (not a function) -- hooking it corrupts data
		g_trampolines[i] = addr;
		if (DetourAttach(&g_trampolines[i], g_stubPage + i * STUB_SIZE) == NO_ERROR)
		{
			g_funcs[i].hooked = true;
			g_hookedAddrs.insert(addr);
			return true;
		}
		g_trampolines[i] = nullptr;
		return false;
	}

	void ProfLog(const char* msg)
	{
		FILE* f = nullptr;
		if (fopen_s(&f, "C:\\Users\\benam\\source\\cpp\\D2MOO\\conformance\\behavioral\\profiler_install_log.txt", "a") == 0 && f)
		{
			fputs(msg, f);
			fputc('\n', f);
			fclose(f); // fclose flushes -- the LAST line survives a crash, pinpointing the culprit
		}
	}

	// Install one subsystem's functions in a single transaction. Safe now that
	// AttachOne gates out aliases (dedup), dispatcher-owned, and DATA exports (the
	// g_pDataTables crash) -- and fast (no per-function file I/O freeze).
	void InstallCategory(const std::string& cat)
	{
		if (!EnsureStubs() || g_installedCats.count(cat))
			return;
		int hooked = 0, skipped = 0;
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		for (int i = 0; i < (int)g_funcs.size(); ++i)
		{
			if (g_funcs[i].category != cat)
				continue;
			if (AttachOne(i)) ++hooked; else ++skipped;
		}
		DetourTransactionCommit();
		g_hookedCount += hooked;
		g_skippedCount += skipped;
		char line[160];
		_snprintf_s(line, sizeof(line), _TRUNCATE, "instrument %s: %d hooked, %d skipped",
			cat.c_str(), hooked, skipped);
		ProfLog(line);
		g_installedCats.insert(cat);
		g_installed = true; // "at least one subsystem instrumented"
	}

	// Instrument EVERY subsystem in one call, still ONE TRANSACTION PER CATEGORY
	// (small blast radius -- a new unhookable case isolates to its subsystem rather
	// than wiping the whole install). Idempotent: InstallCategory's g_installedCats
	// guard skips categories already done, so calling this after some manual
	// per-subsystem installs only fills in the rest. This is the "Instrument All"
	// button's engine.
	void InstallAll()
	{
		if (!EnsureStubs())
			return;
		std::set<std::string> cats;              // distinct categories, table order-agnostic
		for (const auto& f : g_funcs)
			cats.insert(f.category);
		for (const auto& c : cats)
			InstallCategory(c);                  // no-op for already-installed cats
	}

	void ResetCounters()
	{
		std::fill(g_counters.begin(), g_counters.end(), 0ull);
	}
}

// ---------------------------------------------------------------------------
// Data/mechanism accessors -- the UNIFIED browser lives in the "Live Dispatch
// Registry" panel (D2Debugger.LiveDispatch.cpp), which renders this profiler
// table as the full function tree and overlays live-dispatch control on the
// subset that has dispatchers. No separate profiler window.
void D2Prof_EnsureTable() { LoadTable(); }
int  D2Prof_Count() { return (int)g_funcs.size(); }
const char* D2Prof_Name(int i) { return (i >= 0 && i < (int)g_funcs.size()) ? g_funcs[i].name.c_str() : ""; }
const char* D2Prof_Category(int i) { return (i >= 0 && i < (int)g_funcs.size()) ? g_funcs[i].category.c_str() : ""; }
int  D2Prof_Ordinal(int i) { return (i >= 0 && i < (int)g_funcs.size()) ? g_funcs[i].ordinal : -1; }
unsigned int D2Prof_Offset(int i) { return (i >= 0 && i < (int)g_funcs.size()) ? (unsigned int)(g_funcs[i].prefAddr - g_funcs[i].prefBase) : 0xFFFFFFFFu; }
unsigned long long D2Prof_Hits(int i) { return (i >= 0 && i < (int)g_counters.size()) ? g_counters[i] : 0ull; }
bool D2Prof_IsDispatcherOwned(int i)
{
	if (i < 0 || i >= (int)g_funcs.size()) return false;
	return IsDispatcherOwnedOffset(g_funcs[i].prefAddr - g_funcs[i].prefBase);
}
// Maps a dispatcher-owned profiler function to its bridge index (the order MUST
// match LiveDispatchBridge::g_table in patch LiveDispatch_CoordFamily.h). -1 if
// this function has no dispatcher.
int D2Prof_DispatcherBridgeIndex(int i)
{
	if (i < 0 || i >= (int)g_funcs.size()) return -1;
	switch (g_funcs[i].prefAddr - g_funcs[i].prefBase)
	{
	case 0x4dac0: return 0; // DUNGEON_GameTileToClientCoords
	case 0x4d8a0: return 1; // DUNGEON_GameTileToSubtileCoords
	case 0x4d8c0: return 2; // DUNGEON_ClientToGameCoords
	case 0x4db40: return 3; // DUNGEON_GameToClientCoords
	case 0x4db70: return 4; // DUNGEON_GameSubtileToClientCoords
	default: return -1;
	}
}
bool D2Prof_IsHooked(int i) { return (i >= 0 && i < (int)g_funcs.size()) && g_funcs[i].hooked; }
bool D2Prof_Installed() { return g_installed; }
int  D2Prof_Hooked() { return g_hookedCount; }
int  D2Prof_Skipped() { return g_skippedCount; }
void D2Prof_Reset() { ResetCounters(); }
// Per-subsystem instrumentation (the conservative path). A blind mass-install of
// all 1,172 exports at once originally crashed PD2, from TWO now-fixed root causes
// (see DYNAMIC_PROFILER_PLAN.md): (1) ALIASED ordinals -- multiple ordinals map to
// the same address, and hooking one address twice double-patches the prologue
// [fix: dedup by address, g_hookedAddrs]; (2) DATA exports -- non-code exports like
// g_pDataTables get their bytes overwritten with a jump, corrupting the pointer
// [fix: IsExecutableAddr gate]. It was NEVER the raw count. Both fixes are in, so a
// mass-install is now safe; one subsystem at a time just keeps the blast radius
// small if a NEW unhookable case slips through.
void D2Prof_InstallCategory(const char* cat) { InstallCategory(cat ? cat : ""); }
bool D2Prof_IsCategoryInstalled(const char* cat) { return cat && g_installedCats.count(cat) > 0; }
// Instrument every subsystem at once (per-category transactions). Backs the
// "Instrument All" UI button + the /profiler/instrument_all MCP endpoint.
void D2Prof_InstallAll() { InstallAll(); }
