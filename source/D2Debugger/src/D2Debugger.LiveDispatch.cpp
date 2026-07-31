// D2Debugger.LiveDispatch.cpp -- Phase 4 of ../../../conformance/LIVE_DISPATCH_FRAMEWORK_PLAN.md.
//
// A READ-ONLY registry viewer: renders conformance/registry.json (built by
// conformance/tools/build_registry.py) as an ImGui table -- the "promotion
// cockpit" the plan describes, minus live control for now.
//
// WHY READ-ONLY: D2Debugger is built linked against D2MOO's OWN D2Common
// (CMakeLists.txt: target_link_libraries(D2Debugger PRIVATE D2Common ...)),
// but at RUNTIME its imports resolve against whichever module is already
// loaded under the name "D2Common.dll" in the game process -- which is the
// REAL game's D2Common.dll (loaded first, by the game's own LoadLibrary call),
// not D2MOO's separately side-loaded PATCH copy that D2.Detours loads via
// TrueLoadLibraryW into the "patch" folder identity. This is by design
// elsewhere in D2Debugger (it wants to read REAL live game state), but it
// means the live dispatcher's atomics (mode/hits/divergences, declared
// `static` inside D2Common.patch.cpp, i.e. private to OUR patch-copy module)
// are NOT reachable via normal linking from here. Actually toggling live
// dispatcher state from this panel needs an explicit exported API from the
// patch DLL + discovering ITS specific HMODULE.
//
// Phase 4 (2026-07-06): that bridge is now BUILT. The patch D2Common.dll exports
// C functions (D2MOO_LiveDispatch_*, see LiveDispatch_CoordFamily.h). This panel
// enumerates loaded modules, finds the ONE exporting them (the real PD2
// D2Common doesn't -- normal linking would bind to it, hence the explicit
// exported bridge + module search), and calls them for LIVE mode toggling +
// live hit/divergence counters.
#include <imgui.h>
#include "D2Debugger.h"

#include <Windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <mutex>

#include "MemoryModule.h"
// MemoryModule's default alloc/free (global in MemoryModule.c, not in its header).
extern "C" {
	LPVOID MemoryDefaultAlloc(LPVOID, SIZE_T, DWORD, DWORD, void*);
	BOOL   MemoryDefaultFree(LPVOID, SIZE_T, DWORD, void*);
}

namespace
{
	// Same minimal JSON reader shape as conformance/d2moo_conform.cpp (objects/
	// arrays/ints/strings/bool) -- duplicated rather than shared because the two
	// live in separate binaries/build graphs with no common utility target.
	struct JVal
	{
		enum T { NUL, BOOL, NUM, STR, ARR, OBJ } type = NUL;
		long long num = 0; bool b = false; std::string str;
		std::vector<JVal> arr; std::map<std::string, JVal> obj;
		const JVal* find(const char* k) const { auto it = obj.find(k); return it == obj.end() ? nullptr : &it->second; }
		std::string s(const char* k, const char* d = "") const { const JVal* v = find(k); return (v && v->type == STR) ? v->str : d; }
	};
	struct JP
	{
		const char* p; const char* e;
		explicit JP(const std::string& s) : p(s.c_str()), e(s.c_str() + s.size()) {}
		void ws() { while (p < e && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) ++p; }
		JVal val()
		{
			ws(); if (p >= e) return {}; const char c = *p;
			if (c == '{') return obj(); if (c == '[') return arr();
			if (c == '"') { JVal v; v.type = JVal::STR; v.str = str(); return v; }
			if (c == 't' || c == 'f') { JVal v; v.type = JVal::BOOL; v.b = (*p == 't'); p += (*p == 't' ? 4 : 5); return v; }
			if (c == 'n') { p += 4; return {}; } return num();
		}
		JVal obj()
		{
			JVal v; v.type = JVal::OBJ; ++p; ws(); if (p < e && *p == '}') { ++p; return v; }
			while (p < e)
			{
				ws(); std::string k = str(); ws(); if (p < e && *p == ':') ++p; v.obj[k] = val(); ws();
				if (p < e && *p == ',') { ++p; continue; } if (p < e && *p == '}') { ++p; break; } break;
			}
			return v;
		}
		JVal arr()
		{
			JVal v; v.type = JVal::ARR; ++p; ws(); if (p < e && *p == ']') { ++p; return v; }
			while (p < e)
			{
				v.arr.push_back(val()); ws();
				if (p < e && *p == ',') { ++p; continue; } if (p < e && *p == ']') { ++p; break; } break;
			}
			return v;
		}
		std::string str()
		{
			std::string s; if (p < e && *p == '"') ++p;
			while (p < e && *p != '"') { if (*p == '\\' && p + 1 < e) { ++p; s.push_back(*p++); } else s.push_back(*p++); }
			if (p < e && *p == '"') ++p; return s;
		}
		JVal num()
		{
			JVal v; v.type = JVal::NUM; const char* s = p;
			while (p < e && (*p == '-' || *p == '+' || (*p >= '0' && *p <= '9') || *p == '.' || *p == 'e' || *p == 'E')) ++p;
			v.num = (long long)strtoll(std::string(s, p).c_str(), nullptr, 10); return v;
		}
	};

	// SPIKE-ONLY hardcoded absolute path (matches the pattern used by the live
	// dispatcher's own logging in LiveDispatch_CoordFamily.h) -- a real build
	// would resolve this relative to an install/config directory instead.
	const char* kRegistryPath = "C:\\Users\\benam\\source\\cpp\\D2MOO\\conformance\\registry.json";

	struct RegistryRow
	{
		std::string symbol, abi, proof_status, default_mode, notes;
		long long real_ordinal = -1;
		bool has_ordinal = false;
	};

	struct RegistryCache
	{
		std::vector<RegistryRow> rows;
		bool loaded = false;
		bool loadFailed = false;
	};

	RegistryRow RowFromJVal(const JVal& fn)
	{
		RegistryRow r;
		r.symbol = fn.s("d2moo_symbol", "?");
		r.abi = fn.s("abi", "?");
		r.proof_status = fn.s("proof_status", "?");
		r.default_mode = fn.s("default_mode", "?");
		r.notes = fn.s("notes", "");
		if (const JVal* ord = fn.find("real_ordinal"))
		{
			if (ord->type == JVal::NUM) { r.real_ordinal = ord->num; r.has_ordinal = true; }
		}
		return r;
	}

	void LoadRegistry(RegistryCache& cache)
	{
		cache.rows.clear();
		cache.loadFailed = false;

		FILE* f = nullptr;
		if (fopen_s(&f, kRegistryPath, "rb") != 0 || !f) { cache.loadFailed = true; cache.loaded = true; return; }
		std::string text;
		char buf[4096]; size_t n;
		while ((n = fread(buf, 1, sizeof(buf), f)) > 0) text.append(buf, n);
		fclose(f);

		JP jp(text);
		JVal root = jp.val();
		const JVal* functions = root.find("functions");
		if (!functions || functions->type != JVal::ARR) { cache.loadFailed = true; cache.loaded = true; return; }

		for (const JVal& fn : functions->arr)
			cache.rows.push_back(RowFromJVal(fn));
		cache.loaded = true;
	}

	ImVec4 ColorForProofStatus(const std::string& status)
	{
		if (status == "live-shadow-clean") return ImVec4(0.35f, 0.85f, 0.35f, 1.0f); // green: observed live
		if (status == "vectors-passed")    return ImVec4(0.90f, 0.80f, 0.25f, 1.0f); // yellow: offline-proven only
		return ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
	}

	// --- Cross-DLL bridge to the PATCH copy of D2Common.dll's live dispatchers.
	typedef int(__cdecl* GetCountFn)();
	typedef const char* (__cdecl* GetNameFn)(int);
	typedef int(__cdecl* GetModeFn)(int);
	typedef void(__cdecl* SetModeFn)(int, int);
	typedef unsigned long long(__cdecl* GetU64Fn)(int);
	typedef unsigned int(__cdecl* GetOffsetFn)(int);
	typedef void(__cdecl* SetReimplFn)(int, void*);
	typedef int(__cdecl* QuiesceFn)();
	typedef void*(__cdecl* ResolveGameFn)(const char*);
	typedef void*(__cdecl* GetPtrFn)(int); // WS-5 oracle: GetTrampoline / GetReimpl
	// Recorded sampler values: (dispatcher, slot, which) -> value. Scalar rather
	// than an array-out param so nothing has to marshal a buffer across the
	// module boundary -- the bridge only ever passes and returns primitives.
	typedef unsigned int(__cdecl* GetSampleFn)(int, int, int);

	struct Bridge
	{
		bool resolved = false;
		bool available = false;
		GetCountFn getCount = nullptr;
		GetNameFn getName = nullptr;
		GetModeFn getMode = nullptr;
		SetModeFn setMode = nullptr;
		GetU64Fn getHits = nullptr;
		GetU64Fn getDiv = nullptr;
		GetOffsetFn getOffset = nullptr;
		SetReimplFn setReimpl = nullptr;   // WS-1 hot-reload
		QuiesceFn quiesce = nullptr;       // WS-1 hot-reload
		ResolveGameFn resolveGameFn = nullptr; // WS-1.5 verified-address resolver
		GetPtrFn getTrampoline = nullptr;  // WS-5 oracle: raw original
		GetPtrFn getReimpl = nullptr;      // WS-5 oracle: currently-bound reimpl
		// Input diversity. OPTIONAL: an older patch DLL lacks these exports, in
		// which case the JSON omits the fields entirely and battletest_promoter
		// declines to promote rather than promoting on volume alone.
		GetU64Fn getDistinct = nullptr;
		GetModeFn getArgCount = nullptr;   // (int)->int, same shape as getMode
		// Recorded input VALUES (added 2026-07-30). Also OPTIONAL: an older
		// patch DLL lacks them, and /samples then reports values_available
		// false instead of inventing data.
		GetSampleFn getSampleValue = nullptr;
		QuiesceFn getSampleSlotCount = nullptr;   // ()->int, same shape as quiesce
		// --- multi-module ---
		std::string moduleName;            // e.g. "D2Common.dll", "D2Client.dll"
		int indexBase = 0;                 // this module's first GLOBAL index
		int count = 0;                     // dispatchers this module owns
	};
	// EVERY patch module exporting the bridge symbol, in module-snapshot order.
	// Was a single Bridge: ResolveBridge took the FIRST match and stopped, so a
	// second patch DLL (D2Client) would be invisible or would displace D2Common
	// depending on load order. Global dispatcher indices are the concatenation of
	// each module's local range.
	std::vector<Bridge> g_bridges;
	// Primary/capability facade -- the first resolved module. Capability checks
	// (available, quiesce, resolveGameFn...) keep using this unchanged; anything
	// taking a dispatcher INDEX must go through the Br* accessors below.
	Bridge g_bridge;

	// --- global-index routing ------------------------------------------------
	int BrCount()
	{
		int n = 0;
		for (const auto& b : g_bridges) n += b.count;
		return n;
	}
	// Resolve a GLOBAL dispatcher index to its owning module + local index.
	const Bridge* BrFor(int i, int* local)
	{
		for (const auto& b : g_bridges)
			if (i >= b.indexBase && i < b.indexBase + b.count)
			{
				if (local) *local = i - b.indexBase;
				return &b;
			}
		return nullptr;
	}
	const char* BrModule(int i)
	{
		const Bridge* b = BrFor(i, nullptr);
		return b ? b->moduleName.c_str() : "";
	}
	const char* BrName(int i)
	{
		int l = 0; const Bridge* b = BrFor(i, &l);
		return (b && b->getName) ? b->getName(l) : "";
	}
	int BrGetMode(int i)
	{
		int l = 0; const Bridge* b = BrFor(i, &l);
		return (b && b->getMode) ? b->getMode(l) : 0;
	}
	void BrSetMode(int i, int m)
	{
		int l = 0; const Bridge* b = BrFor(i, &l);
		if (b && b->setMode) b->setMode(l, m);
	}
	unsigned long long BrHits(int i)
	{
		int l = 0; const Bridge* b = BrFor(i, &l);
		return (b && b->getHits) ? b->getHits(l) : 0ull;
	}
	unsigned long long BrDiv(int i)
	{
		int l = 0; const Bridge* b = BrFor(i, &l);
		return (b && b->getDiv) ? b->getDiv(l) : 0ull;
	}
	unsigned int BrOffset(int i)
	{
		int l = 0; const Bridge* b = BrFor(i, &l);
		return (b && b->getOffset) ? b->getOffset(l) : 0xFFFFFFFFu;
	}
	bool BrHasDiversity(int i)
	{
		const Bridge* b = BrFor(i, nullptr);
		return b && b->getDistinct && b->getArgCount;
	}
	// Recorded sampler value, module-local index resolved like the others.
	unsigned int BrSampleValue(int i, int slot, int which)
	{
		int l = 0; const Bridge* b = BrFor(i, &l);
		return (b && b->getSampleValue) ? b->getSampleValue(l, slot, which) : 0u;
	}
	int BrSampleSlots(int i)
	{
		int l = 0; const Bridge* b = BrFor(i, &l);
		return (b && b->getSampleSlotCount) ? b->getSampleSlotCount() : 0;
	}
	bool BrHasSamples(int i)
	{
		int l = 0; const Bridge* b = BrFor(i, &l);
		return b && b->getSampleValue && b->getSampleSlotCount;
	}
	unsigned long long BrDistinct(int i)
	{
		int l = 0; const Bridge* b = BrFor(i, &l);
		return (b && b->getDistinct) ? b->getDistinct(l) : 0ull;
	}
	int BrArgCount(int i)
	{
		int l = 0; const Bridge* b = BrFor(i, &l);
		return (b && b->getArgCount) ? b->getArgCount(l) : -1;
	}
	void* BrTrampoline(int i)
	{
		int l = 0; const Bridge* b = BrFor(i, &l);
		return (b && b->getTrampoline) ? b->getTrampoline(l) : nullptr;
	}
	void* BrReimpl(int i)
	{
		int l = 0; const Bridge* b = BrFor(i, &l);
		return (b && b->getReimpl) ? b->getReimpl(l) : nullptr;
	}
	void BrSetReimpl(int i, void* fn)
	{
		int l = 0; const Bridge* b = BrFor(i, &l);
		if (b && b->setReimpl) b->setReimpl(l, fn);
	}

	// --- MemoryModule custom import resolver (WS-1.5 detail A2) ---
	// The provider is loaded from an in-memory buffer; its imports are resolved
	// here. Imports from "D2Common.dll" go through the VERIFIED-address resolver
	// (never the scrambled export table); everything else (CRT, kernel32, ...)
	// uses the real loader.
	const HCUSTOMMODULE kD2CommonSentinel = (HCUSTOMMODULE)(uintptr_t)1;

	HCUSTOMMODULE __cdecl MM_LoadLibrary(LPCSTR name, void*)
	{
		if (name && _stricmp(name, "D2Common.dll") == 0)
			return kD2CommonSentinel; // resolve its procs by verified address
		return (HCUSTOMMODULE)LoadLibraryA(name);
	}
	FARPROC __cdecl MM_GetProcAddress(HCUSTOMMODULE mod, LPCSTR name, void*)
	{
		if (mod == kD2CommonSentinel)
			return g_bridge.resolveGameFn ? (FARPROC)g_bridge.resolveGameFn(name) : nullptr;
		return GetProcAddress((HMODULE)mod, name);
	}
	void __cdecl MM_FreeLibrary(HCUSTOMMODULE mod, void*)
	{
		if (mod != kD2CommonSentinel && mod)
			FreeLibrary((HMODULE)mod);
	}

	// WS-1 reimpl-provider hot-reload state -- now an IN-MEMORY module (no temp
	// files, no LoadLibrary path caching -- see GRADUATED_CONFORMANCE_PIPELINE_PLAN.md
	// detail A2 and the MemoryModule research).
	HMEMORYMODULE g_provider = nullptr;
	char g_providerStatus[128] = "provider: not loaded";
	int g_reloadSeq = 0;
	const char* kProviderPath =
		"C:\\Users\\benam\\source\\cpp\\D2MOO\\build-1.13c\\patch\\D2MOO_ReimplProvider.dll";

	// The reload protocol (detail A): save modes -> quiesce+drain ->
	// MemoryFreeLibrary -> read the freshly-built provider BYTES from disk ->
	// MemoryLoadLibraryEx with the verified-address import resolver -> re-resolve
	// exports + SetReimpl -> restore modes. Loading from a memory buffer sidesteps
	// the file lock, temp-file dance, and same-path module cache entirely.
	void ReloadProvider()
	{
		if (!g_bridge.available || !g_bridge.quiesce || !g_bridge.setReimpl || !g_bridge.getName)
		{
			_snprintf_s(g_providerStatus, sizeof(g_providerStatus), _TRUNCATE, "provider: bridge unavailable");
			return;
		}
		const int n = BrCount();
		std::vector<int> prevModes(n);
		for (int i = 0; i < n; ++i)
			prevModes[i] = BrGetMode(i);

		bool quiesced = true;
		for (const auto& br : g_bridges)
			if (br.quiesce && br.quiesce() != 1) quiesced = false;
		if (!quiesced)
		{
			_snprintf_s(g_providerStatus, sizeof(g_providerStatus), _TRUNCATE,
				"provider: QUIESCE TIMEOUT -- reload aborted (a reimpl is stuck)");
			return;
		}
		if (g_provider) { MemoryFreeLibrary(g_provider); g_provider = nullptr; }

		// Read the freshly-built provider bytes (file NOT kept open/mapped, so the
		// build can overwrite it freely next time).
		std::vector<unsigned char> bytes;
		{
			FILE* f = nullptr;
			if (fopen_s(&f, kProviderPath, "rb") != 0 || !f)
			{
				_snprintf_s(g_providerStatus, sizeof(g_providerStatus), _TRUNCATE,
					"provider: cannot open %s", kProviderPath);
				for (int i = 0; i < n; ++i) BrSetMode(i, prevModes[i]);
				return;
			}
			unsigned char buf[8192]; size_t r;
			while ((r = fread(buf, 1, sizeof(buf), f)) > 0) bytes.insert(bytes.end(), buf, buf + r);
			fclose(f);
		}

		g_provider = MemoryLoadLibraryEx(bytes.data(), bytes.size(),
			MemoryDefaultAlloc, MemoryDefaultFree,
			MM_LoadLibrary, MM_GetProcAddress, MM_FreeLibrary, nullptr);
		if (!g_provider)
		{
			_snprintf_s(g_providerStatus, sizeof(g_providerStatus), _TRUNCATE,
				"provider: MemoryLoadLibrary FAILED (err %lu)", GetLastError());
			for (int i = 0; i < n; ++i) BrSetMode(i, prevModes[i]);
			return;
		}
		++g_reloadSeq;

		// Inject the verified-address resolver so reimpls can read REAL game
		// globals/functions BY NAME (provider_runtime.h dependency injection) --
		// no hardcoded addresses, no scrambled export table. Must run BEFORE any
		// reimpl is called.
		typedef void(__cdecl* ProviderInitFn)(void*);
		if (auto initFn = (ProviderInitFn)MemoryGetProcAddress(g_provider, "D2MOO_Provider_Init"))
			if (g_bridge.resolveGameFn) initFn((void*)g_bridge.resolveGameFn);

		int bound = 0;
		for (int i = 0; i < n; ++i)
		{
			void* fn = (void*)MemoryGetProcAddress(g_provider, BrName(i));
			if (fn) { BrSetReimpl(i, fn); ++bound; }
		}
		for (int i = 0; i < n; ++i)
			BrSetMode(i, prevModes[i]);

		_snprintf_s(g_providerStatus, sizeof(g_providerStatus), _TRUNCATE,
			"provider: mem-loaded #%d, %d/%d bound (modes restored)", g_reloadSeq, bound, n);
	}

	// offset -> bridge index, rebuilt from the bridge each frame it's available.
	// DATA-DRIVEN: any dispatcher added to the patch bridge auto-appears here, so
	// newly-ported equivalents become shadow-selectable with no UI code change.
	// Map a PROFILER function offset to its dispatcher index.
	//
	// MODULE-SCOPED (fixed 2026-07-29): every caller feeds this a
	// D2Prof_Offset(), and the profiler enumerates D2Common exports ONLY. Once
	// multi-bridge added D2Client dispatchers to the same global index space,
	// an unscoped offset compare could return a D2CLIENT dispatcher for a
	// D2COMMON profiler function whose offset happens to collide -- wiring that
	// row's mode/divergence controls to an unrelated function in a different
	// binary. Offsets are only meaningful relative to their own module, so
	// restrict the search to the module the offset actually came from.
	int BridgeIndexForOffset(uint32_t off, const char* moduleName = "D2Common.dll")
	{
		if (!g_bridge.available || !g_bridge.getOffset)
			return -1;
		const int n = BrCount();
		for (int i = 0; i < n; ++i)
			if (BrOffset(i) == off && _stricmp(BrModule(i), moduleName) == 0)
				return i;
		return -1;
	}

	// The REAL bug behind the 2026-07-29 "206 dispatchers instead of 103"
	// finding: `if (g_bridge.resolved) return; g_bridge.resolved = true;` is a
	// check-then-set with NO lock, so two threads calling ResolveBridge() for
	// the first time concurrently (this HTTP server handles requests on
	// multiple threads) can both read resolved==false before either writes
	// true, and both run the full enumeration + g_bridges.push_back() pass --
	// permanently double-populating the global vector for the rest of the
	// process's lifetime. A within-pass GetProcAddress dedup (added earlier
	// the same day) does not help: each racing thread's OWN pass never sees a
	// duplicate hModule, so nothing in a single pass ever collides.
	// std::call_once is the correct fix -- exactly one execution, no matter
	// how many threads call in concurrently, no manual locking to get wrong.
	static std::once_flag g_bridgeResolveOnce;

	void ResolveBridgeImpl()
	{
		// The real game's D2Common.dll and D2MOO's patch copy share the base name
		// "D2Common.dll", so GetModuleHandle is ambiguous. Disambiguate by the ONE
		// export only the patch has: whichever loaded module answers
		// D2MOO_LiveDispatch_GetCount is the patch copy.
		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
		if (snap == INVALID_HANDLE_VALUE)
			return;
		MODULEENTRY32W me{};
		me.dwSize = sizeof(me);
		// Toolhelp32's module snapshot can list the SAME loaded module twice
		// (a documented quirk; confirmed live 2026-07-29: every D2Common AND
		// D2Client dispatcher showed up at two global indices reporting
		// byte-identical counters -- same underlying static storage, resolved
		// via GetProcAddress on the same hModule twice). Not a live hazard
		// (either duplicate index operates on the one real dispatcher
		// correctly), but it doubles the reported count and makes any caller
		// that iterates 0..count-1 do every promotion/refutation twice per
		// poll. Dedupe by the resolved GetCount proc address -- two hModule
		// values pointing at the same loaded image resolve to the identical
		// function pointer.
		std::vector<GetCountFn> seenGetCount;
		if (Module32FirstW(snap, &me))
		{
			do
			{
				auto f = (GetCountFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetCount");
				if (f && std::find(seenGetCount.begin(), seenGetCount.end(), f) == seenGetCount.end())
				{
					seenGetCount.push_back(f);
					// Collect EVERY patch module, don't stop at the first. Each
					// owns a contiguous slice of the global index space.
					Bridge b;
					b.getCount = f;
					b.getName = (GetNameFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetName");
					b.getMode = (GetModeFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetMode");
					b.setMode = (SetModeFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_SetMode");
					b.getHits = (GetU64Fn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetHits");
					b.getDiv = (GetU64Fn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetDivergences");
					b.getOffset = (GetOffsetFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetOffset");
					b.setReimpl = (SetReimplFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_SetReimpl");
					b.quiesce = (QuiesceFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_QuiesceForReload");
					b.resolveGameFn = (ResolveGameFn)GetProcAddress(me.hModule, "D2MOO_ResolveGameFn");
					b.getTrampoline = (GetPtrFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetTrampoline");
					b.getReimpl = (GetPtrFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetReimpl");
					b.getDistinct = (GetU64Fn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetDistinctInputs");
					b.getSampleValue = (GetSampleFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetSampleValue");
					b.getSampleSlotCount = (QuiesceFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetSampleSlotCount");
					b.getArgCount = (GetModeFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetArgCount");
					b.available = b.getName && b.getMode && b.setMode &&
						b.getHits && b.getDiv && b.getOffset;
					if (b.available)
					{
						char nm[MAX_PATH]{};
						WideCharToMultiByte(CP_UTF8, 0, me.szModule, -1, nm, sizeof(nm) - 1, nullptr, nullptr);
						b.moduleName = nm;
						b.count = b.getCount();
						b.indexBase = BrCount();     // sum of what is already registered
						g_bridges.push_back(b);
						// First module doubles as the capability facade.
						if (g_bridges.size() == 1) g_bridge = b;
					}
				}
			} while (Module32NextW(snap, &me));
		}
		CloseHandle(snap);
	}

	// Public entry point every call site already uses. std::call_once makes
	// the "resolve exactly once" guarantee actually true under concurrent
	// callers, replacing the racy check-then-set on g_bridge.resolved.
	void ResolveBridge()
	{
		std::call_once(g_bridgeResolveOnce, ResolveBridgeImpl);
	}

}

// Dynamic-profiler accessors (D2Debugger.profiler.cpp) -- the full function
// table + live hit counts that this unified browser renders as the tree.
void D2Prof_EnsureTable();
int  D2Prof_Count();
const char* D2Prof_Name(int i);
const char* D2Prof_Category(int i);
int  D2Prof_Ordinal(int i);
unsigned int D2Prof_Offset(int i);
unsigned long long D2Prof_Hits(int i);
bool D2Prof_IsHooked(int i);
bool D2Prof_Installed();
int  D2Prof_Hooked();
int  D2Prof_Skipped();
void D2Prof_Reset();
void D2Prof_InstallCategory(const char* cat);
bool D2Prof_IsCategoryInstalled(const char* cat);
void D2Prof_InstallAll();

// General ABI call marshaller (D2Debugger.oracle.cpp) -- design detail B.
extern "C" uint32_t D2Oracle_Call(void* fn, int cc, const uint32_t* a, int n);
extern "C" uint64_t D2Oracle_Call64(void* fn, int cc, const uint32_t* a, int n); // int64 (edx:eax) ret
extern "C" void D2Oracle_CallRegs(void* fn, uint32_t* io); // register-explicit (io: eax,ecx,edx,ebx,esi,edi)
// Game-thread call queue (D2Debugger.gtqueue.cpp): marshal a call onto the game
// thread. Returns 1=ok, 0=timed out (not in-world), -1=faulted (SEH-caught).
extern "C" int D2Gt_Call(void* fn, int cc, const uint32_t* args, int nargs, int ret64, uint64_t* out, int timeoutMs);
extern "C" int D2Gt_Call2(void* fnA, void* fnB, int cc, const uint32_t* args, int nargs, int ret64, uint64_t* outA, uint64_t* outB, int timeoutMs);

// Live game-object handle capture (D2Debugger.capture.cpp) -- the oracle passes
// a real captured live object to a stateful function via arg kind "handle".
// Retargetable at runtime via POST /capture so a HOT target can be found live.
void* D2Capture_LastUnit();
extern "C" int D2Capture_FillDistinct(void** out, int max); // one object per dwType, for branch coverage
unsigned D2Capture_Count();
bool D2Capture_Attach(unsigned int offset, bool useEcx);
unsigned D2Capture_Offset();
bool D2Capture_Attached();

// Menu-launch action (D2Debugger.action.cpp) -- the MUTATING "drive the game"
// capability: launches the currently-highlighted single-player character by
// calling D2Launch's SelectCharacterByIndex(0) on the game thread (the real
// menu launch path, live-confirmed 2026-07-07), drained via a hook on D2Win's
// per-frame RenderMainFrame.
extern "C" bool D2Action_ReadCharSelectState(uint32_t* outCharIndex, uint32_t* outListLoaded);
extern "C" int  D2Action_LaunchSelectedCharacter(int difficulty, int timeoutMs);
extern "C" int  D2Action_MainMenuSinglePlayer(int timeoutMs);
extern "C" int  D2Action_ExitGame(int timeoutMs);
extern "C" int  D2Action_SaveAndExitToMenu(int timeoutMs);
extern "C" int  D2Action_LoadCharacterByName(const char* name, int difficulty, int timeoutMs);
extern "C" int  D2Action_ListCharactersJson(char* buf, int bufSize);
extern "C" bool D2Action_IsPumpHookInstalled();

// Asset overlay archive registration (D2Debugger.assetreload.cpp) -- registers a
// high-priority patch.mpq into the live Storm search list so edited assets
// override the base game (the Asset Studio override channel; see
// doc/AssetStudioPlan.md). SFileOpenArchive marshalled onto the game thread.
extern "C" int  D2Asset_RegisterArchive(const char* path, int priority, int timeoutMs);
extern "C" int  D2Asset_CloseArchive(int timeoutMs);
extern "C" int  D2Asset_StatusJson(char* buf, int bufSize);
extern "C" int  D2Asset_SpawnItem(const char* code, int drop, int dest,
                                  int quality, int qualRow, int identify, int timeoutMs);
extern "C" int  D2Asset_PickupDropped(unsigned int guid, int timeoutMs);
extern "C" unsigned int D2Asset_LastDroppedGuid();
extern "C" int  D2Asset_OpenInventory(int timeoutMs);
extern "C" int  D2Asset_DriveInventory(int nClose, int timeoutMs);
extern "C" int  D2Asset_ItemText(unsigned int guid, char* utf8Buf, int bufSize, int timeoutMs);
extern "C" int  D2Asset_HoverXY(int gameX, int gameY, int* outX, int* outY);
// Virtual input (D2Debugger.vinput.cpp) -- drive the game without moving the
// operator's real pointer. See that file for the measured import set.
extern "C" void D2VInput_SetEnabled(int on);
extern "C" int  D2VInput_IsEnabled();
extern "C" void D2VInput_SetScreenPos(int x, int y);
extern "C" void D2VInput_GetScreenPos(int* x, int* y);
extern "C" void D2VInput_SetKey(int vk, int down);
extern "C" int  D2VInput_GetKey(int vk);
extern "C" int  D2VInput_MoveToGameXY(int gameX, int gameY, int* outX, int* outY);
extern "C" void D2VInput_GetCounters(unsigned long* c, unsigned long* a, unsigned long* k);
extern "C" int  D2VInput_PostMouseMove(int clientX, int clientY);
// Clean frame capture (D2Debugger.vcapture.cpp).
extern "C" int  D2Capture_WriteFramePng(const char* path, int withOverlay,
                                        int timeoutMs, int* outW, int* outH);
extern "C" unsigned long D2Capture_FrameCount();
extern "C" void D2Capture_LastGeometry(int* srcSignedH, int* hDst, int* hSrc, int* usedTopDown);
extern "C" int  D2Probe_Report(char* buf, int cch);
extern "C" int  D2Asset_PeekDwords(const char* module, unsigned int rva, int count, unsigned int* out);
extern "C" int  D2Asset_ReadBytes(const char* module, unsigned int rva, int len, unsigned char* out);
extern "C" int  D2Asset_PokeDword(const char* module, unsigned int rva, unsigned int value);
extern "C" int  D2Asset_DumpItemStats(unsigned int guid, char* buf, int bufSize);

// =====================================================================
// WS-5: MCP control surface (GRADUATED_CONFORMANCE_PIPELINE_PLAN.md).
//
// An in-process HTTP API so an external agent (the d2debugger-mcp bridge)
// can READ dispatcher/profiler state and DRIVE shadow-proving WITHOUT a
// human clicking the ImGui panel -- the keystone for autonomous proving.
// The raw Winsock server lives in D2Debugger.mcp.cpp; it calls the router
// below, which (living in this TU) has direct access to the file-scope
// bridge, ReloadProvider(), and the profiler accessors.
// =====================================================================
namespace
{
	std::mutex g_mcpMutex; // serialize mutating ops (reload, set-all, instrument)

	void JsonEscapeInto(const std::string& in, std::string& out)
	{
		for (char c : in)
		{
			switch (c)
			{
			case '"':  out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case '\n': out += "\\n";  break;
			case '\r': out += "\\r";  break;
			case '\t': out += "\\t";  break;
			default:
				if ((unsigned char)c < 0x20)
				{
					char b[8]; _snprintf_s(b, sizeof(b), _TRUNCATE, "\\u%04x", (unsigned)(unsigned char)c);
					out += b;
				}
				else out.push_back(c);
			}
		}
	}
	// JSON string literal (with surrounding quotes) from a raw string.
	std::string JStr(const std::string& s) { std::string o = "\""; JsonEscapeInto(s, o); o += "\""; return o; }

	const char* ModeName(int m) { return m == 2 ? "shadow" : (m == 1 ? "reimpl" : "original"); }

	int ParseMode(const JVal* m)
	{
		if (!m) return -1;
		if (m->type == JVal::NUM) { int x = (int)m->num; return (x >= 0 && x <= 2) ? x : -1; }
		if (m->type == JVal::STR)
		{
			if (m->str == "original" || m->str == "Original" || m->str == "0") return 0;
			if (m->str == "reimpl"   || m->str == "Reimpl"   || m->str == "1") return 1;
			if (m->str == "shadow"   || m->str == "Shadow"   || m->str == "2") return 2;
		}
		return -1;
	}

	// "/a/b/c?x=1" -> ["a","b","c"] (query stripped).
	std::vector<std::string> SplitPath(const std::string& path)
	{
		std::vector<std::string> segs;
		size_t i = 0;
		while (i < path.size())
		{
			if (path[i] == '/') { ++i; continue; }
			size_t j = path.find('/', i);
			if (j == std::string::npos) j = path.size();
			std::string seg = path.substr(i, j - i);
			size_t q = seg.find('?');
			if (q != std::string::npos) seg = seg.substr(0, q);
			if (!seg.empty()) segs.push_back(seg);
			i = j;
		}
		return segs;
	}

	// Effective hits (dispatcher-owned funcs report via the bridge, others via
	// the profiler) -- same rule the ImGui panel uses.
	unsigned long long McpEffHits(int i)
	{
		const int bi = BridgeIndexForOffset(D2Prof_Offset(i));
		if (bi >= 0 && g_bridge.available) return BrHits(bi);
		return D2Prof_Hits(i);
	}

	std::string DispatcherJson(int i)
	{
		std::string name = g_bridge.getName ? BrName(i) : "";
		char buf[768];
		// distinct_inputs/arg_count are OMITTED when the patch DLL predates the
		// sampler. That absence is meaningful: battletest_promoter fails closed on
		// it, so a stale patch stalls promotion instead of promoting on volume
		// alone -- which is the evidence SHIPPING_PROMOTION_PLAN calls worthless.
		char diversity[96] = "";
		if (BrHasDiversity(i))
		{
			_snprintf_s(diversity, sizeof(diversity), _TRUNCATE,
				",\"distinct_inputs\":%llu,\"arg_count\":%d",
				(unsigned long long)BrDistinct(i), BrArgCount(i));
		}
		// `hooked` = the Detours trampoline is non-null, i.e. DllPreLoadHook's
		// ApplyPatchAction actually installed for this dispatcher. Added
		// 2026-07-29 because without it, a dispatcher reading hits=0 is
		// genuinely AMBIGUOUS between "the hook never installed" and "the
		// function is simply never called" -- and distinguishing those
		// otherwise needs a debugger attach, which the elevated game blocks.
		// A permanent, zero-cost answer to a question that cost most of a
		// session to ask the hard way.
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"{\"index\":%d,\"name\":%s,\"module\":%s,\"offset\":%u,\"mode\":%d,"
			"\"modeName\":\"%s\",\"hooked\":%s,\"hits\":%llu,\"divergences\":%llu%s}",
			i, JStr(name).c_str(), JStr(BrModule(i)).c_str(), BrOffset(i), BrGetMode(i),
			ModeName(BrGetMode(i)),
			BrTrampoline(i) ? "true" : "false",
			(unsigned long long)BrHits(i), (unsigned long long)BrDiv(i), diversity);
		return buf;
	}

	std::string ErrJson(const char* msg)
	{
		std::string o = "{\"ok\":false,\"error\":"; o += JStr(msg); o += "}"; return o;
	}

	// LIVE-BASE RESOLUTION (2026-07-30). Every absolute address in this stack was
	// authored from Ghidra's IMAGE BASE, on the unstated assumption that the module
	// loads there. That holds for D2Common (0x6fd50000) and D2Game (0x6fc20000) --
	// and NOT for D2Client, which the live process maps at 0x03600000 while
	// 0x6fab0000 is not mapped at all. So every D2Client oracle call `call`ed
	// unmapped memory, faulted, and came back as {"error":"handler-exception"}
	// -- which fun-doc's taxonomy files as `marshal_fault`, i.e. "wrong
	// callconv/slot-count or bad pointer arg". 104 D2Client functions were
	// TERMINALLY retired on that verdict without their reimpl ever being executed
	// once (a zero-arg void setter "failed an ABI check" on a single vector).
	//
	// The fix is to stop trusting the file's preferred base: a spec supplies
	// "module" + "rva" and we resolve against the RUNTIME base. GuardedTarget then
	// refuses to call anything that isn't mapped executable, so a base mistake can
	// never again masquerade as a verdict about the function.
	void* ResolveModuleRva(const char* module, unsigned int rva)
	{
		uintptr_t base = (uintptr_t)GetModuleHandleA(module);
		return base ? (void*)(base + rva) : nullptr;
	}

	// True when addr points at committed, executable memory. VirtualQuery is the
	// cheap authority here -- it answers before we hand the address to a `call`,
	// which is the whole point: an unmapped target must be reported as a BAD
	// TARGET, not discovered as an SEH fault indistinguishable from a bad ABI.
	bool IsCallableAddress(const void* addr)
	{
		if (!addr) return false;
		MEMORY_BASIC_INFORMATION mbi{};
		if (VirtualQuery(addr, &mbi, sizeof(mbi)) != sizeof(mbi)) return false;
		if (mbi.State != MEM_COMMIT) return false;
		if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
		const DWORD exec = PAGE_EXECUTE | PAGE_EXECUTE_READ
			| PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
		return (mbi.Protect & exec) != 0;
	}

	// --- design detail B: general-oracle spec helpers ---
	// Keep in sync with OracleCC in D2Debugger.oracle.cpp.
	int ParseCallConv(const std::string& s)
	{
		if (s == "cdecl"    || s == "__cdecl")    return 0;
		if (s == "stdcall"  || s == "__stdcall"  || s.empty()) return 1; // stdcall default
		if (s == "fastcall" || s == "__fastcall") return 2;
		if (s == "thiscall" || s == "__thiscall") return 3;
		return -1;
	}

	// Dispatcher index for a function NAME (so the oracle prefers the raw
	// trampoline for a hooked fn -- calling its verified address would route
	// through the mode-dependent Thunk). -1 if the fn has no dispatcher.
	int DispatcherIndexForName(const char* name)
	{
		if (!g_bridge.available || !g_bridge.getName || !name) return -1;
		const int n = BrCount();
		for (int i = 0; i < n; ++i)
		{
			const char* nm = BrName(i);
			if (nm && strcmp(nm, name) == 0) return i;
		}
		return -1;
	}

	// A type-gate patch on a synth/synth2 buffer: force `*(base+off) (w bytes) = imm`
	// so a getter's precondition (`if (pUnit->dwType != 4) return 0;`) takes the
	// SUCCESS path and actually reads the discriminating field. depth 0 = the primary
	// buffer (flat synth, or the pointer table of synth2); depth 1 = synth2 secondary.
	struct OGate { int depth = 0; int off = 0; uint32_t imm = 0; int w = 4; };
	struct OArg { std::string id; bool isBuf = false; bool isHandle = false; bool isSynth = false; bool isSynth2 = false; int bytes = 4; std::vector<OGate> gates; };

	// Comparison mask for a sub-dword return (byte/short leave stale upper bits in
	// EAX; callers use only the low byte/word). Full width otherwise.
	uint64_t RetMask(const std::string& ret)
	{
		if (ret == "u8"  || ret == "i8")  return 0xFFull;
		if (ret == "u16" || ret == "i16") return 0xFFFFull;
		if (ret == "u64" || ret == "i64") return 0xFFFFFFFFFFFFFFFFull;
		return 0xFFFFFFFFull; // u32/i32/ptr/void
	}

	// Read a u32 from live game memory under SEH -- for the COVERAGE PROBE (reading a
	// captured object's dispatch field, e.g. dwType at +0, so we can tell which
	// BRANCH each object exercised). Own function, no C++ unwinding objects (C2712).
	// A bad/stale pointer yields 0xFFFFFFFF (an obvious out-of-domain marker) instead
	// of faulting.
	uint32_t SafeReadU32(const void* p)
	{
		__try { return *(const volatile uint32_t*)p; }
		__except (EXCEPTION_EXECUTE_HANDLER) { return 0xFFFFFFFFu; }
	}

	// GP register name -> D2Oracle_CallRegs io index. -1 if not a GP reg.
	int RegIndex(const std::string& r)
	{
		if (r == "EAX" || r == "eax") return 0;
		if (r == "ECX" || r == "ecx") return 1;
		if (r == "EDX" || r == "edx") return 2;
		if (r == "EBX" || r == "ebx") return 3;
		if (r == "ESI" || r == "esi") return 4;
		if (r == "EDI" || r == "edi") return 5;
		return -1;
	}
}

// Called by the Winsock server (D2Debugger.mcp.cpp) for each request. Returns
// the JSON response body. External linkage so the server TU can call it.
std::string D2Mcp_HandleRequest(const std::string& method, const std::string& path, const std::string& body)
{
	ResolveBridge();
	D2Prof_EnsureTable();
	std::vector<std::string> seg = SplitPath(path);
	if (seg.empty())
		return ErrJson("no route");

	// GET /status -- overall health, safe to poll.
	if (seg.size() == 1 && seg[0] == "status")
	{
		bool d2LaunchResolved = GetModuleHandleA("D2Launch.dll") != nullptr;
		uint32_t charIdx = 0xFFFFFFFFu, listLoaded = 0;
		bool charSelReady = D2Action_ReadCharSelectState(&charIdx, &listLoaded);
		// specModuleRva advertises that /oracle honours "module"+"rva" (runtime-base
		// resolution) and gates a call on IsCallableAddress. A client MUST check it
		// before live-proving a relocated module: an older build silently ignores
		// those fields, falls back to the absolute "addr", and turns a wrong base
		// into an SEH fault that reads as an ABI verdict (2026-07-30, D2Client).
		char buf[1024];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"{\"ok\":true,\"bridge\":%s,\"dispatchers\":%d,\"provider\":%s,\"reloadSeq\":%d,"
			"\"profilerHooked\":%d,\"profilerSkipped\":%d,\"functions\":%d,"
			"\"capturedHandle\":\"0x%08x\",\"captureCount\":%u,"
			"\"specModuleRva\":true,"
			"\"d2LaunchResolved\":%s,\"menuPumpHooked\":%s,"
			"\"charSelectReady\":%s,\"selectedCharIndex\":%d,\"charListLoaded\":%s}",
			g_bridge.available ? "true" : "false",
			g_bridge.available ? BrCount() : 0,
			JStr(g_providerStatus).c_str(), g_reloadSeq,
			D2Prof_Hooked(), D2Prof_Skipped(), D2Prof_Count(),
			(unsigned)(uintptr_t)D2Capture_LastUnit(), D2Capture_Count(),
			d2LaunchResolved ? "true" : "false",
			D2Action_IsPumpHookInstalled() ? "true" : "false",
			charSelReady ? "true" : "false", (int)charIdx,
			listLoaded ? "true" : "false");
		return buf;
	}

	// GET /modules -- every loaded module's name, RUNTIME base and size.
	//
	// Added 2026-07-30 because its absence made a whole bug class invisible. The
	// conformance stack assumed "live base == Ghidra image base" everywhere, and
	// nothing in the process could report otherwise: D2Client is mapped at
	// 0x03600000 while Ghidra has it at 0x6fab0000, so every D2Client oracle call
	// hit unmapped memory and came back as a generic SEH "handler-exception" that
	// the taxonomy read as a wrong ABI. Diagnosing it required deriving the base by
	// hand from a relocated MOV operand. This route makes the runtime layout a
	// first-class, one-request fact, and lets tooling detect a relocated module
	// (and re-queue what it falsely failed) without guessing.
	if (seg[0] == "modules" && seg.size() == 1 && method == "GET")
	{
		std::string o = "{\"ok\":true,\"modules\":[";
		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
		if (snap == INVALID_HANDLE_VALUE) return ErrJson("CreateToolhelp32Snapshot failed");
		MODULEENTRY32 me{}; me.dwSize = sizeof(me);
		bool first = true;
		for (BOOL more = Module32First(snap, &me); more; more = Module32Next(snap, &me))
		{
			char buf[MAX_MODULE_NAME32 + 96];
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"%s{\"name\":%s,\"base\":%u,\"size\":%u}",
				first ? "" : ",", JStr(me.szModule).c_str(),
				(unsigned int)(uintptr_t)me.modBaseAddr, (unsigned int)me.modBaseSize);
			o += buf;
			first = false;
		}
		CloseHandle(snap);
		o += "]}";
		return o;
	}

	// /dispatchers  (GET list | POST /dispatchers/mode set-all)
	if (seg[0] == "dispatchers")
	{
		if (!g_bridge.available) return ErrJson("bridge unavailable");
		const int n = BrCount();
		if (seg.size() == 1 && method == "GET")
		{
			std::string o = "{\"ok\":true,\"count\":" + std::to_string(n) + ",\"dispatchers\":[";
			for (int i = 0; i < n; ++i) { if (i) o += ","; o += DispatcherJson(i); }
			o += "]}";
			return o;
		}
		if (seg.size() == 2 && seg[1] == "mode" && method == "POST")
		{
			JP jp(body); JVal v = jp.val();
			int mode = ParseMode(v.find("mode"));
			if (mode < 0) return ErrJson("bad mode (want 0|1|2 or original|reimpl|shadow)");
			std::lock_guard<std::mutex> lk(g_mcpMutex);
			for (int i = 0; i < n; ++i) BrSetMode(i, mode);
			return std::string("{\"ok\":true,\"set\":") + std::to_string(n) + ",\"mode\":\"" + ModeName(mode) + "\"}";
		}
		return ErrJson("bad /dispatchers route");
	}

	// /dispatcher/{i}  (GET detail | POST /dispatcher/{i}/mode)
	if (seg[0] == "dispatcher" && seg.size() >= 2)
	{
		if (!g_bridge.available) return ErrJson("bridge unavailable");
		const int n = BrCount();
		int i = atoi(seg[1].c_str());
		if (i < 0 || i >= n) return ErrJson("dispatcher index out of range");
		if (seg.size() == 2 && method == "GET")
			return std::string("{\"ok\":true,\"dispatcher\":") + DispatcherJson(i) + "}";
		// GET /dispatcher/{i}/samples -- the ACTUAL argument values recorded by
		// the diversity sampler. Turns "needs 4 more distinct inputs" into "you
		// have already seen these ids", which is what makes a coverage gap
		// targetable instead of guesswork.
		if (seg.size() == 3 && seg[2] == "samples" && method == "GET")
		{
			if (!BrHasSamples(i))
				return std::string("{\"ok\":true,\"values_available\":false,\"note\":")
					+ JStr("patch DLL predates value recording; rebuild + redeploy")
					+ "}";
			const int slots = BrSampleSlots(i);
			std::string o = "{\"ok\":true,\"values_available\":true,\"name\":";
			o += JStr(BrName(i));
			o += ",\"arg_count\":" + std::to_string(BrArgCount(i));
			o += ",\"distinct_inputs\":" + std::to_string(BrDistinct(i));
			o += ",\"samples\":[";
			bool firstS = true;
			for (int sl = 0; sl < slots; ++sl)
			{
				// which==2 is the slot hash: 0 means the slot was never filled.
				// The table is sparse, so this skip is required.
				if (BrSampleValue(i, sl, 2) == 0u) continue;
				if (!firstS) o += ",";
				firstS = false;
				o += "{\"arg0\":" + std::to_string(BrSampleValue(i, sl, 0));
				o += ",\"arg1\":" + std::to_string(BrSampleValue(i, sl, 1)) + "}";
			}
			o += "]}";
			return o;
		}
		if (seg.size() == 3 && seg[2] == "mode" && method == "POST")
		{
			JP jp(body); JVal v = jp.val();
			int mode = ParseMode(v.find("mode"));
			if (mode < 0) return ErrJson("bad mode (want 0|1|2 or original|reimpl|shadow)");
			std::lock_guard<std::mutex> lk(g_mcpMutex);
			BrSetMode(i, mode);
			return std::string("{\"ok\":true,\"dispatcher\":") + DispatcherJson(i) + "}";
		}
		return ErrJson("bad /dispatcher route");
	}

	// POST /reimpl/reload -- hot-reload the reimpl-provider (WS-1).
	if (seg[0] == "reimpl" && seg.size() == 2 && seg[1] == "reload" && method == "POST")
	{
		std::lock_guard<std::mutex> lk(g_mcpMutex);
		ReloadProvider();
		return std::string("{\"ok\":true,\"provider\":") + JStr(g_providerStatus) +
			",\"reloadSeq\":" + std::to_string(g_reloadSeq) + "}";
	}

	// /capture -- live game-object handle capture control (stateful frontier).
	//   GET  -> current target offset + count + handle.
	//   POST {"offset":N,"ecx":bool} -> (re)attach the capture hook to D2Common+N.
	// Lets an agent HUNT for a hot non-inlined target live, no rebuild.
	if (seg[0] == "capture" && seg.size() == 1)
	{
		if (method == "POST")
		{
			JP jp(body); JVal v = jp.val();
			const JVal* jo = v.find("offset");
			if (!jo || jo->type != JVal::NUM) return ErrJson("want {offset:N[,ecx:bool]}");
			bool ecx = false;
			if (const JVal* je = v.find("ecx")) ecx = (je->type == JVal::BOOL && je->b);
			std::lock_guard<std::mutex> lk(g_mcpMutex);
			bool ok = D2Capture_Attach((unsigned)jo->num, ecx);
			char buf[192];
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"{\"ok\":%s,\"attached\":%s,\"offset\":%u,\"ecx\":%s}",
				ok ? "true" : "false", D2Capture_Attached() ? "true" : "false",
				D2Capture_Offset(), ecx ? "true" : "false");
			return buf;
		}
		char buf[224];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"{\"ok\":true,\"attached\":%s,\"offset\":%u,\"captureCount\":%u,\"handle\":\"0x%08x\"}",
			D2Capture_Attached() ? "true" : "false", D2Capture_Offset(),
			D2Capture_Count(), (unsigned)(uintptr_t)D2Capture_LastUnit());
		return buf;
	}

	// /profiler/...
	if (seg[0] == "profiler" && seg.size() >= 2)
	{
		const int n = D2Prof_Count();

		// GET /profiler/subsystems -- per-subsystem rollup + installed flag.
		if (seg[1] == "subsystems" && method == "GET")
		{
			struct Agg { int count = 0; unsigned long long hits = 0; };
			std::map<std::string, Agg> cats;
			for (int i = 0; i < n; ++i)
			{
				Agg& a = cats[D2Prof_Category(i)];
				a.count++; a.hits += McpEffHits(i);
			}
			std::string o = "{\"ok\":true,\"subsystems\":[";
			bool first = true;
			for (auto& kv : cats)
			{
				if (!first) o += ","; first = false;
				o += "{\"name\":" + JStr(kv.first) +
					",\"functions\":" + std::to_string(kv.second.count) +
					",\"hits\":" + std::to_string(kv.second.hits) +
					",\"installed\":" + (D2Prof_IsCategoryInstalled(kv.first.c_str()) ? "true" : "false") + "}";
			}
			o += "]}";
			return o;
		}

		// POST /profiler/instrument  {"subsystem":"SKILLS"}
		if (seg[1] == "instrument" && method == "POST")
		{
			JP jp(body); JVal v = jp.val();
			std::string sub = v.s("subsystem");
			if (sub.empty()) return ErrJson("missing subsystem");
			std::lock_guard<std::mutex> lk(g_mcpMutex);
			D2Prof_InstallCategory(sub.c_str());
			return std::string("{\"ok\":true,\"instrumented\":") + JStr(sub) +
				",\"installed\":" + (D2Prof_IsCategoryInstalled(sub.c_str()) ? "true" : "false") + "}";
		}

		// POST /profiler/instrument_all -- instrument EVERY subsystem at once
		// (per-category transactions; idempotent). Same engine as the "Instrument
		// All" UI button. Returns the resulting hooked/skipped totals.
		if (seg[1] == "instrument_all" && method == "POST")
		{
			std::lock_guard<std::mutex> lk(g_mcpMutex);
			D2Prof_InstallAll();
			return std::string("{\"ok\":true,\"instrumented\":\"all\",\"hooked\":") +
				std::to_string(D2Prof_Hooked()) + ",\"skipped\":" +
				std::to_string(D2Prof_Skipped()) + "}";
		}

		// GET /profiler/functions/{subsystem} -- functions in one subsystem.
		if (seg[1] == "functions" && seg.size() == 3 && method == "GET")
		{
			std::string want = seg[2];
			std::string o = "{\"ok\":true,\"subsystem\":" + JStr(want) + ",\"functions\":[";
			bool first = true;
			for (int i = 0; i < n; ++i)
			{
				if (want != D2Prof_Category(i)) continue;
				if (!first) o += ","; first = false;
				const int bi = BridgeIndexForOffset(D2Prof_Offset(i));
				char buf[512];
				_snprintf_s(buf, sizeof(buf), _TRUNCATE,
					"{\"ordinal\":%d,\"name\":%s,\"offset\":%u,\"hits\":%llu,\"hooked\":%s,\"hasEquiv\":%s}",
					D2Prof_Ordinal(i), JStr(D2Prof_Name(i)).c_str(), D2Prof_Offset(i),
					McpEffHits(i), D2Prof_IsHooked(i) ? "true" : "false",
					(bi >= 0 && g_bridge.available) ? "true" : "false");
				o += buf;
			}
			o += "]}";
			return o;
		}
		return ErrJson("bad /profiler route");
	}

	// POST /call/{i}  -- direct-call oracle (WS-5c). Body is either a single
	// {"x":N,"y":N} or a batch {"vectors":[{"x":N,"y":N},...]}. Calls BOTH the
	// raw original (Detours trampoline) and the currently-bound reimpl with each
	// input and compares -- a deterministic, mode-independent proof with CHOSEN
	// inputs (vs. passively waiting for the game to exercise a shadow path).
	//
	// ABI NOTE: marshals the coord family's shared `void __stdcall(int*,int*)`
	// shape -- correct for all 5 current dispatchers. Generalizing to arbitrary
	// ABIs is design detail B (deferred). A faulting call is caught by the
	// server's SEH wrapper and returned as {"error":"handler-exception"} rather
	// than crashing the game.
	if (seg[0] == "call" && seg.size() == 2 && method == "POST")
	{
		if (!g_bridge.available || !g_bridge.getTrampoline || !g_bridge.getReimpl)
			return ErrJson("oracle unavailable (patch missing GetTrampoline/GetReimpl -- rebuild D2Common patch)");
		const int n = BrCount();
		int i = atoi(seg[1].c_str());
		if (i < 0 || i >= n) return ErrJson("dispatcher index out of range");

		JP jp(body); JVal v = jp.val();
		std::vector<std::pair<int, int>> inputs;
		const JVal* vecs = v.find("vectors");
		if (vecs && vecs->type == JVal::ARR)
		{
			for (const JVal& e : vecs->arr)
			{
				const JVal* jx = e.find("x"); const JVal* jy = e.find("y");
				if (jx && jx->type == JVal::NUM && jy && jy->type == JVal::NUM)
					inputs.emplace_back((int)jx->num, (int)jy->num);
			}
		}
		else
		{
			const JVal* jx = v.find("x"); const JVal* jy = v.find("y");
			if (jx && jx->type == JVal::NUM && jy && jy->type == JVal::NUM)
				inputs.emplace_back((int)jx->num, (int)jy->num);
		}
		if (inputs.empty()) return ErrJson("want {x,y} or {vectors:[{x,y},...]}");
		if (inputs.size() > 4096) return ErrJson("too many vectors (max 4096)");

		typedef void(__stdcall* CoordFn)(int*, int*);
		CoordFn orig = (CoordFn)BrTrampoline(i);
		CoordFn re   = (CoordFn)BrReimpl(i);
		if (!orig) return ErrJson("trampoline null (dispatcher not hooked yet)");
		if (!re)   return ErrJson("reimpl null (no equivalent bound)");

		int matches = 0;
		std::string results = "[";
		for (size_t k = 0; k < inputs.size(); ++k)
		{
			int oX = inputs[k].first, oY = inputs[k].second;
			int rX = inputs[k].first, rY = inputs[k].second;
			orig(&oX, &oY);
			re(&rX, &rY);
			const bool m = (oX == rX && oY == rY);
			if (m) ++matches;
			char row[256];
			_snprintf_s(row, sizeof(row), _TRUNCATE,
				"%s{\"in\":{\"x\":%d,\"y\":%d},\"original\":{\"x\":%d,\"y\":%d},"
				"\"reimpl\":{\"x\":%d,\"y\":%d},\"match\":%s}",
				k ? "," : "", inputs[k].first, inputs[k].second, oX, oY, rX, rY, m ? "true" : "false");
			results += row;
		}
		results += "]";

		const int cnt = (int)inputs.size();
		std::string o = "{\"ok\":true,\"name\":" + JStr(BrName(i)) +
			",\"count\":" + std::to_string(cnt) +
			",\"matches\":" + std::to_string(matches) +
			",\"mismatches\":" + std::to_string(cnt - matches) +
			",\"allMatch\":" + (matches == cnt ? "true" : "false") +
			",\"results\":" + results + "}";
		return o;
	}

	// POST /action/launch-character -- MUTATING "drive the game" capability.
	// Launches the CURRENTLY-HIGHLIGHTED single-player character by calling
	// D2Launch's SelectCharacterByIndex(0) on the game thread (the real menu
	// launch path, live-confirmed 2026-07-07 -- exactly what clicking the
	// "Normal" difficulty button does). Requires {"confirm":true}; refuses if
	// not at a character-select screen with a character highlighted. The
	// game-thread queue is drained by the D2Win RenderMainFrame menu hook.
	if (seg[0] == "action" && seg.size() == 2 && seg[1] == "launch-character" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		const JVal* jc = v.find("confirm");
		if (!jc || jc->type != JVal::BOOL || !jc->b)
			return ErrJson("refused: POST body must include \"confirm\":true -- this action MUTATES "
			               "live game state (launches the selected single-player character)");

		if (!GetModuleHandleA("D2Launch.dll"))
			return ErrJson("D2Launch.dll not resolved (module not loaded?)");
		uint32_t idx = 0xFFFFFFFFu, listLoaded = 0;
		if (!D2Action_ReadCharSelectState(&idx, &listLoaded))
		{
			char buf[224];
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"{\"ok\":false,\"error\":\"not ready to launch -- be at the character-select screen with a "
				"character HIGHLIGHTED\",\"selectedCharIndex\":%d,\"charListLoaded\":%s}",
				(int)idx, listLoaded ? "true" : "false");
			return buf;
		}

		int difficulty = 0; // 0=Normal, 1=Nightmare, 2=Hell (must be UNLOCKED)
		if (const JVal* jd = v.find("difficulty")) if (jd->type == JVal::NUM) difficulty = (int)jd->num;
		int timeoutMs = 4000;
		if (const JVal* jt = v.find("timeoutMs")) if (jt->type == JVal::NUM) timeoutMs = (int)jt->num;
		std::lock_guard<std::mutex> lk(g_mcpMutex);
		int gs = D2Action_LaunchSelectedCharacter(difficulty, timeoutMs);
		if (gs == 0)  return ErrJson("game-thread call timed out (menu pump not firing -- is a D2Win menu screen active?)");
		if (gs == -1) return ErrJson("game-thread call FAULTED (SEH-caught)");
		if (gs == -2) return ErrJson("D2Launch.dll not resolved");
		if (gs == -3) return ErrJson("no character highlighted / char list not loaded");
		char buf[192];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"{\"ok\":true,\"launched\":true,\"selectedCharIndex\":%d,"
			"\"note\":\"SelectCharacterByIndex(0) executed on the game thread; the game should load the "
			"highlighted character\"}", (int)idx);
		return buf;
	}

	// GET /action/list-characters -- enumerate the single-player character-select
	// list (read-only). Returns [{index,name,class},...]. Use to see available
	// characters (and the exact stored name to pass to /action/load-character).
	if (seg[0] == "action" && seg.size() == 2 && seg[1] == "list-characters" && method == "GET")
	{
		if (!GetModuleHandleA("D2Launch.dll")) return ErrJson("D2Launch.dll not resolved");
		static char list[8192];
		int n = D2Action_ListCharactersJson(list, sizeof(list));
		if (n < 0) return ErrJson("character list not loaded (be at the character-select screen)");
		std::string o = "{\"ok\":true,\"count\":" + std::to_string(n) + ",\"characters\":";
		o += list; o += "}";
		return o;
	}

	// POST /action/load-character  {"name":"MyChar"[,"confirm":true]} -- find the
	// named single-player character in the char-select list, highlight it, and
	// launch it. Must be at the character-select screen. Requires confirm:true.
	if (seg[0] == "action" && seg.size() == 2 && seg[1] == "load-character" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		const JVal* jc = v.find("confirm");
		if (!jc || jc->type != JVal::BOOL || !jc->b)
			return ErrJson("refused: POST body must include \"confirm\":true");
		std::string name = v.s("name");
		if (name.empty()) return ErrJson("missing \"name\" (character to load)");
		if (!GetModuleHandleA("D2Launch.dll")) return ErrJson("D2Launch.dll not resolved");
		int difficulty = 0; // 0=Normal, 1=Nightmare, 2=Hell (must be UNLOCKED on the char)
		if (const JVal* jd = v.find("difficulty")) if (jd->type == JVal::NUM) difficulty = (int)jd->num;
		int timeoutMs = 4000;
		if (const JVal* jt = v.find("timeoutMs")) if (jt->type == JVal::NUM) timeoutMs = (int)jt->num;
		std::lock_guard<std::mutex> lk(g_mcpMutex);
		int r = D2Action_LoadCharacterByName(name.c_str(), difficulty, timeoutMs);
		if (r == -2) return ErrJson("D2Launch.dll not resolved");
		if (r == -3) return ErrJson("game-thread call timed out (be at the character-select screen)");
		if (r == -4) return ErrJson("game-thread call FAULTED (SEH-caught)");
		if (r == -1) return ErrJson("no single-player character with that name in the list");
		char buf[224];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"{\"ok\":true,\"launched\":true,\"matchedIndex\":%d,\"name\":%s,\"difficulty\":%d}",
			r, JStr(name).c_str(), difficulty);
		return buf;
	}

	// POST /action/main-menu-singleplayer -- click "Single Player" on the title
	// screen (D2Launch StartSinglePlayerMode() on the game thread -> transitions
	// to character-select). Requires {"confirm":true}.
	if (seg[0] == "action" && seg.size() == 2 && seg[1] == "main-menu-singleplayer" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		const JVal* jc = v.find("confirm");
		if (!jc || jc->type != JVal::BOOL || !jc->b)
			return ErrJson("refused: POST body must include \"confirm\":true");
		if (!GetModuleHandleA("D2Launch.dll")) return ErrJson("D2Launch.dll not resolved");
		int timeoutMs = 4000;
		if (const JVal* jt = v.find("timeoutMs")) if (jt->type == JVal::NUM) timeoutMs = (int)jt->num;
		std::lock_guard<std::mutex> lk(g_mcpMutex);
		int gs = D2Action_MainMenuSinglePlayer(timeoutMs);
		if (gs == 0)  return ErrJson("game-thread call timed out (menu pump not firing -- be at the title/menu)");
		if (gs == -1) return ErrJson("game-thread call FAULTED (SEH-caught)");
		if (gs == -2) return ErrJson("D2Launch.dll not resolved");
		return "{\"ok\":true,\"note\":\"StartSinglePlayerMode() executed; title screen should transition to "
		       "character-select\"}";
	}

	// POST /action/exit-game -- exit Diablo (D2Win quit levers on the game
	// thread: the menu loop exits and does not restart). Requires {"confirm":true}.
	if (seg[0] == "action" && seg.size() == 2 && seg[1] == "exit-game" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		const JVal* jc = v.find("confirm");
		if (!jc || jc->type != JVal::BOOL || !jc->b)
			return ErrJson("refused: POST body must include \"confirm\":true -- this CLOSES the game");
		if (!GetModuleHandleA("D2Win.dll")) return ErrJson("D2Win.dll not resolved");
		int timeoutMs = 4000;
		if (const JVal* jt = v.find("timeoutMs")) if (jt->type == JVal::NUM) timeoutMs = (int)jt->num;
		std::lock_guard<std::mutex> lk(g_mcpMutex);
		int gs = D2Action_ExitGame(timeoutMs);
		if (gs == 0)  return ErrJson("game-thread call timed out (menu pump not firing -- be at a menu screen)");
		if (gs == -1) return ErrJson("game-thread call FAULTED (SEH-caught)");
		if (gs == -2) return ErrJson("D2Win.dll not resolved");
		return "{\"ok\":true,\"note\":\"quit levers set; the game should close\"}";
	}

	// POST /action/exit-to-menu -- in-game "Save and Exit Game" -> back to
	// character-select. Calls D2Client HandleSaveAndExitDialogConfirm() on the
	// game thread (drained by the IN-WORLD capture pump -- be in a game).
	// Requires {"confirm":true}.
	if (seg[0] == "action" && seg.size() == 2 && seg[1] == "exit-to-menu" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		const JVal* jc = v.find("confirm");
		if (!jc || jc->type != JVal::BOOL || !jc->b)
			return ErrJson("refused: POST body must include \"confirm\":true (saves + leaves the game)");
		if (!GetModuleHandleA("D2Client.dll")) return ErrJson("D2Client.dll not resolved");
		int timeoutMs = 4000;
		if (const JVal* jt = v.find("timeoutMs")) if (jt->type == JVal::NUM) timeoutMs = (int)jt->num;
		std::lock_guard<std::mutex> lk(g_mcpMutex);
		int gs = D2Action_SaveAndExitToMenu(timeoutMs);
		if (gs == 0)  return ErrJson("game-thread call timed out (in-world pump not firing -- must be IN a game)");
		if (gs == -1) return ErrJson("game-thread call FAULTED (SEH-caught)");
		if (gs == -2) return ErrJson("D2Client.dll not resolved");
		if (gs == -6) return ErrJson("a dialog was already open in-game -- close it first, then retry");
		return "{\"ok\":true,\"note\":\"save-exit dialog opened + confirmed; the game saves and returns to "
		       "character-select\"}";
	}

	// /asset -- PD2 Asset Studio overlay archive registration (the override channel).
	//   GET  /asset/status                -> whether an overlay archive is registered.
	//   POST /asset/register {path, priority?, confirm:true}  -> SFileOpenArchive on the
	//        game thread; priority defaults to 9000 (> the game's patch_d2.mpq @ 5000, so
	//        our archive wins every shared file). Register at the MENU, then re-enter the
	//        game (soft reload) so item art loads from our archive.
	//   POST /asset/close {confirm:true}  -> SFileCloseArchive (revert to stock files).
	if (seg[0] == "asset" && seg.size() == 2 && seg[1] == "status" && method == "GET")
	{
		static char sbuf[768];
		D2Asset_StatusJson(sbuf, sizeof(sbuf));
		return sbuf;
	}
	if (seg[0] == "asset" && seg.size() == 2 && seg[1] == "register" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		const JVal* jc = v.find("confirm");
		if (!jc || jc->type != JVal::BOOL || !jc->b)
			return ErrJson("refused: POST body must include \"confirm\":true (registers an overlay "
			               "archive into the live game's file search list)");
		const JVal* jpath = v.find("path");
		if (!jpath || jpath->type != JVal::STR || jpath->str.empty())
			return ErrJson("want {\"path\":\"C:/.../patch.mpq\",\"priority\":9000,\"confirm\":true}");
		int priority = 9000; // > patch_d2.mpq's 5000
		if (const JVal* jpr = v.find("priority")) if (jpr->type == JVal::NUM) priority = (int)jpr->num;
		int timeoutMs = 6000;
		if (const JVal* jt = v.find("timeoutMs")) if (jt->type == JVal::NUM) timeoutMs = (int)jt->num;
		std::lock_guard<std::mutex> lk(g_mcpMutex);
		int gs = D2Asset_RegisterArchive(jpath->str.c_str(), priority, timeoutMs);
		if (gs == 0)  return ErrJson("game-thread call timed out (no pump firing -- be at a menu or in-world)");
		if (gs == -1) return ErrJson("game-thread call FAULTED (SEH-caught)");
		if (gs == -2) return ErrJson("Storm.dll / SFileOpenArchive not resolved");
		if (gs == -3) return ErrJson("SFileOpenArchive failed (bad path, or archive not a valid MPQ?)");
		static char rbuf[768];
		D2Asset_StatusJson(rbuf, sizeof(rbuf));
		return rbuf;
	}
	if (seg[0] == "asset" && seg.size() == 2 && seg[1] == "close" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		const JVal* jc = v.find("confirm");
		if (!jc || jc->type != JVal::BOOL || !jc->b)
			return ErrJson("refused: POST body must include \"confirm\":true (closes the overlay archive)");
		int timeoutMs = 6000;
		if (const JVal* jt = v.find("timeoutMs")) if (jt->type == JVal::NUM) timeoutMs = (int)jt->num;
		std::lock_guard<std::mutex> lk(g_mcpMutex);
		int gs = D2Asset_CloseArchive(timeoutMs);
		if (gs == 0)  return ErrJson("game-thread call timed out (no pump firing)");
		if (gs == -1) return ErrJson("game-thread call FAULTED (SEH-caught)");
		if (gs == -2) return ErrJson("SFileCloseArchive not resolved");
		return "{\"ok\":true,\"note\":\"overlay archive closed; stock file resolution restored\"}";
	}

	// POST /showcase/item {"code":"uap","dest":"inventory","confirm":true} -- summon a base item to
	// inspect its art/stats. dest "feet" (default) drops it at the player's feet (click to pick up).
	// dest "inventory" drops it AND replays the native 0x16 pickup packet so the server picks it up
	// into the inventory with full client sync (see doc/AssetStudioPlan.md §27) -- then hover it to
	// read all properties. "code" is the 1-4 char base-item code. Must be IN a game.
	if (seg[0] == "showcase" && seg.size() == 2 && seg[1] == "item" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		const JVal* jc = v.find("confirm");
		if (!jc || jc->type != JVal::BOOL || !jc->b)
			return ErrJson("refused: POST body must include \"confirm\":true (spawns a live item)");
		const JVal* jcode = v.find("code");
		if (!jcode || jcode->type != JVal::STR || jcode->str.empty())
			return ErrJson("want {\"code\":\"uap\",\"dest\":\"inventory\",\"confirm\":true}  (uap = Shako base)");
		bool toInv = false;
		if (const JVal* jde = v.find("dest")) if (jde->type == JVal::STR && jde->str == "inventory") toInv = true;
		int timeoutMs = 4000;
		if (const JVal* jt = v.find("timeoutMs")) if (jt->type == JVal::NUM) timeoutMs = (int)jt->num;
		// Forced quality/row (all optional):
		//   "quality": ITEMQUAL_* (5=set, 7=unique, 4=magic, 6=rare, 3=superior, 2=normal, 1=low). 0/absent = normal roll.
		//   "setRow"/"uniqueRow": force a specific setitems/uniqueitems game row for that quality. "code" MUST be that row's base.
		//   "identify": true -> mark the dropped item identified (needed for a unique's own inventory art).
		// setRow is kept for back-compat and implies quality=set; uniqueRow implies quality=unique.
		int quality = 0, qualRow = -1;
		if (const JVal* jq = v.find("quality")) if (jq->type == JVal::NUM) quality = (int)jq->num;
		if (const JVal* js = v.find("setRow"))    if (js->type == JVal::NUM) { qualRow = (int)js->num; if (!quality) quality = 5; }
		if (const JVal* ju = v.find("uniqueRow")) if (ju->type == JVal::NUM) { qualRow = (int)ju->num; if (!quality) quality = 7; }
		int identify = 0;
		if (const JVal* ji = v.find("identify")) if (ji->type == JVal::BOOL) identify = ji->b ? 1 : 0;
		std::lock_guard<std::mutex> lk(g_mcpMutex);
		int gs = D2Asset_SpawnItem(jcode->str.c_str(), /*drop*/1, /*dest*/0, quality, qualRow, identify, timeoutMs);
		if (gs == 0)  return ErrJson("game-thread call timed out (in-world pump not firing -- must be IN a game)");
		if (gs == -1) return ErrJson("game-thread call FAULTED (SEH-caught)");
		if (gs == -2) return ErrJson("server game not captured yet -- be IN a game a moment (the per-frame "
		                             "server tick populates it), then retry");
		if (gs == -3) return ErrJson("could not resolve the server player from the game (client list / player "
		                             "not ready) -- retry");
		if (gs == -4) return ErrJson("D2Common/D2Game module not resolved");
		if (gs == -5) return ErrJson("unknown item code (check the 1-4 char base-item code)");
		if (gs != 1)  return ErrJson("item create/drop failed");
		if (!toInv)
			return std::string("{\"ok\":true,\"spawned\":true,\"code\":\"") + jcode->str +
			       "\",\"dest\":\"feet\",\"note\":\"dropped at your feet\"}";
		// dest=inventory: let the client create its ground copy, then replay the 0x16 pickup packet.
		unsigned int guid = D2Asset_LastDroppedGuid();
		if (guid == 0) return ErrJson("item dropped but could not be located to pick up -- it is on the ground");
		Sleep(300);
		int pk = D2Asset_PickupDropped(guid, timeoutMs);
		if (pk == 1)  return std::string("{\"ok\":true,\"spawned\":true,\"code\":\"") + jcode->str +
		                     "\",\"dest\":\"inventory\",\"note\":\"picked up into inventory (0x16 replay)\"}";
		if (pk == 0)  return ErrJson("pickup packet timed out on the game thread -- item is on the ground");
		if (pk == -1) return ErrJson("pickup packet send FAULTED (SEH) -- item is on the ground");
		if (pk == -2) return ErrJson("D2Client.dll not resolved for the pickup send");
		return ErrJson("pickup replay failed -- item is on the ground");
	}

	// POST /showcase/item-stats {"guid":"0x..."} (or omit guid to use the last-spawned item) --
	// dump the server item's StatList as JSON [{id,sub,val}] for verifying .txt stat edits.
	if (seg[0] == "showcase" && seg.size() == 2 && seg[1] == "item-stats" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		unsigned int guid = 0;
		if (const JVal* jg = v.find("guid")) {
			if (jg->type == JVal::NUM) guid = (unsigned int)jg->num;
			else if (jg->type == JVal::STR) guid = (unsigned int)strtoul(jg->str.c_str(), nullptr, 0);
		}
		if (guid == 0) guid = D2Asset_LastDroppedGuid();
		if (guid == 0) return ErrJson("no guid given and no last-spawned item -- spawn one first");
		static char sbuf[8192];
		int r = D2Asset_DumpItemStats(guid, sbuf, sizeof(sbuf));
		if (r >= 0) return std::string(sbuf);
		if (r == -1) return ErrJson("stat dump faulted (SEH) -- retry");
		if (r == -2) return ErrJson("server game not captured / bad guid");
		return ErrJson("item not found in the server item hash for that guid");
	}

	// POST /asset/peek {"module":"D2Client.dll","rva":1162804,"count":4} -- read-only dword peek at
	// module+rva (diagnostic for probing client UI-state globals). rva is decimal or 0x-hex string.
	if (seg[0] == "asset" && seg.size() == 2 && seg[1] == "peek" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		const JVal* jm = v.find("module");
		std::string mod = (jm && jm->type == JVal::STR) ? jm->str : std::string("D2Client.dll");
		unsigned int rva = 0;
		if (const JVal* jr = v.find("rva")) {
			if (jr->type == JVal::NUM) rva = (unsigned int)jr->num;
			else if (jr->type == JVal::STR) rva = (unsigned int)strtoul(jr->str.c_str(), nullptr, 0);
		}
		int count = 1;
		if (const JVal* jcnt = v.find("count")) if (jcnt->type == JVal::NUM) count = (int)jcnt->num;
		unsigned int vals[8] = {0};
		int got = D2Asset_PeekDwords(mod.c_str(), rva, count, vals);
		std::string s = "{\"ok\":true,\"module\":\"" + mod + "\",\"got\":" + std::to_string(got) + ",\"vals\":[";
		for (int i = 0; i < got; ++i) { char b[16]; _snprintf_s(b, sizeof(b), _TRUNCATE, "%u", vals[i]); if (i) s += ","; s += b; }
		s += "]}";
		return s;
	}

	// POST /asset/read {"module":"D2Client.dll","rva":"0x20f20","len":512} -- bulk read,
	// returned as lowercase hex. Up to 4096 bytes per call, vs /asset/peek's 8 dwords,
	// so a whole-module live-vs-file diff is one call per function instead of dozens.
	// "got" may be < len when the read runs off the end of a mapped section; the valid
	// prefix is still returned rather than nothing.
	if (seg[0] == "asset" && seg.size() == 2 && seg[1] == "read" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		const JVal* jm = v.find("module");
		std::string mod = (jm && jm->type == JVal::STR) ? jm->str : std::string("D2Client.dll");
		unsigned int rva = 0;
		if (const JVal* jr = v.find("rva")) {
			if (jr->type == JVal::NUM) rva = (unsigned int)jr->num;
			else if (jr->type == JVal::STR) rva = (unsigned int)strtoul(jr->str.c_str(), nullptr, 0);
		}
		int len = 256;
		if (const JVal* jl = v.find("len")) if (jl->type == JVal::NUM) len = (int)jl->num;
		if (len < 1) len = 1;
		if (len > 4096) len = 4096;
		std::vector<unsigned char> buf((size_t)len);
		const int got = D2Asset_ReadBytes(mod.c_str(), rva, len, buf.data());
		std::string hex;
		hex.reserve((size_t)got * 2);
		static const char* kHex = "0123456789abcdef";
		for (int i = 0; i < got; ++i) { hex += kHex[buf[i] >> 4]; hex += kHex[buf[i] & 0xF]; }
		return "{\"ok\":true,\"module\":" + JStr(mod) + ",\"rva\":" + std::to_string(rva) +
		       ",\"got\":" + std::to_string(got) + ",\"hex\":\"" + hex + "\"}";
	}

	// POST /asset/poke {"module":"D2Client.dll","rva":"0x11c284","value":1} -- write one dword at
	// module+rva (diagnostic for driving client UI-state globals). SEH-guarded.
	if (seg[0] == "asset" && seg.size() == 2 && seg[1] == "poke" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		const JVal* jm = v.find("module");
		std::string mod = (jm && jm->type == JVal::STR) ? jm->str : std::string("D2Client.dll");
		unsigned int rva = 0, val = 0;
		if (const JVal* jr = v.find("rva")) {
			if (jr->type == JVal::NUM) rva = (unsigned int)jr->num;
			else if (jr->type == JVal::STR) rva = (unsigned int)strtoul(jr->str.c_str(), nullptr, 0);
		}
		if (const JVal* jv = v.find("value")) {
			if (jv->type == JVal::NUM) val = (unsigned int)jv->num;
			else if (jv->type == JVal::STR) val = (unsigned int)strtoul(jv->str.c_str(), nullptr, 0);
		}
		int r = D2Asset_PokeDword(mod.c_str(), rva, val);
		if (r == 1)  return std::string("{\"ok\":true,\"wrote\":") + std::to_string(val) + "}";
		if (r == 0)  return ErrJson("module not resolved");
		return ErrJson("poke faulted (SEH)");
	}

	// POST /showcase/open-inventory {"confirm":true[,"close":true]} -- open (or close) the in-game
	// inventory panel (UI panel 1) through CLIENT_ProcessUIStateChange on the game thread, so the
	// panel + item art + tooltips can be screenshotted without a real keypress. Idempotent; verifies
	// the render-gate flag g_adwUIPanelActive[1]. See doc/AssetStudioPlan.md §28 (session-2 fix).
	if (seg[0] == "showcase" && seg.size() == 2 && seg[1] == "open-inventory" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		const JVal* jc = v.find("confirm");
		if (!jc || jc->type != JVal::BOOL || !jc->b)
			return ErrJson("refused: POST body must include \"confirm\":true");
		bool close = false;
		if (const JVal* jcl = v.find("close")) close = (jcl->type == JVal::BOOL && jcl->b);
		int timeoutMs = 4000;
		if (const JVal* jt = v.find("timeoutMs")) if (jt->type == JVal::NUM) timeoutMs = (int)jt->num;
		std::lock_guard<std::mutex> lk(g_mcpMutex);
		int gs = D2Asset_DriveInventory(close ? 1 : 0, timeoutMs);
		if (gs == 1)  return std::string("{\"ok\":true,\"inventoryOpen\":") + (close ? "false" : "true") + "}";
		if (gs == 0)  return ErrJson("game-thread call timed out (must be IN a game)");
		if (gs == -1) return ErrJson("game-thread call FAULTED (SEH-caught)");
		if (gs == -2) return ErrJson("D2Client.dll not resolved");
		if (gs == -3) return ErrJson("UI state machine refused (render-gate flag unchanged) -- in a menu/dialog?");
		return ErrJson("open-inventory failed");
	}

	// POST /showcase/item-text {"guid":"0x.."} (or omit guid = first item in the player's inventory) --
	// run the game's own item name/description builder (CLIENT_BuildItemDescriptionTooltip @6fb414f0)
	// on the CLIENT item and return the localized hover-name text. Machine-readable half of the hover
	// tooltip; pair with /showcase/item-stats for the numeric stat lines.
	if (seg[0] == "showcase" && seg.size() == 2 && seg[1] == "item-text" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		unsigned int guid = 0;
		if (const JVal* jg = v.find("guid")) {
			if (jg->type == JVal::NUM) guid = (unsigned int)jg->num;
			else if (jg->type == JVal::STR) guid = (unsigned int)strtoul(jg->str.c_str(), nullptr, 0);
		}
		int timeoutMs = 4000;
		if (const JVal* jt = v.find("timeoutMs")) if (jt->type == JVal::NUM) timeoutMs = (int)jt->num;
		std::lock_guard<std::mutex> lk(g_mcpMutex);
		static char tbuf[2048];
		int gs = D2Asset_ItemText(guid, tbuf, sizeof(tbuf), timeoutMs);
		if (gs == 1)
		{
			std::string esc; esc.reserve(512);
			for (const char* p = tbuf; *p; ++p) {
				unsigned char c = (unsigned char)*p;
				if (c == '"' || c == '\\') { esc += '\\'; esc += (char)c; }
				else if (c == '\n') esc += "\\n";
				else if (c < 0x20) { char b[8]; _snprintf_s(b, sizeof(b), _TRUNCATE, "\\u%04x", c); esc += b; }
				else esc += (char)c;
			}
			char head[64]; _snprintf_s(head, sizeof(head), _TRUNCATE, "{\"ok\":true,\"guid\":\"0x%08x\",", guid);
			return std::string(head) + "\"text\":\"" + esc + "\"}";
		}
		if (gs == 0)  return ErrJson("game-thread call timed out (must be IN a game)");
		if (gs == -1) return ErrJson("faulted / client player not resolved");
		if (gs == -2) return ErrJson("no client inventory / D2Client not resolved");
		if (gs == -3) return ErrJson("item not found in the CLIENT inventory for that guid (picked up yet?)");
		return ErrJson("item-text failed");
	}

	// GET /capture/probe -- which API actually presents the frame, and who calls it.
	// Call it twice a second or so apart: `perSec` is derived from the gap between
	// calls, and the per-frame present is the one running at the frame rate.
	if (seg[0] == "capture" && seg.size() == 2 && seg[1] == "probe" && method == "GET")
	{
		static char rep[4096];
		D2Probe_Report(rep, (int)sizeof(rep));
		return std::string(rep);
	}

	// POST /capture/frame {"path":"C:\tmp\shot.png"[,"withOverlay":false][,"timeoutMs":3000]}
	// Write a PNG of the game's frame. By default the image is CLEAN -- captured
	// before ImGui composites, so no debug panels, no cursor, no window chrome,
	// at the render resolution rather than the window size. There is no other way
	// to get one: the overlay draws into the game's OWN backbuffer, so every
	// external capture has the panels baked in.
	// withOverlay:true captures the composited view instead (what you see).
	if (seg[0] == "capture" && seg.size() == 2 && seg[1] == "frame" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		const std::string path = v.s("path");
		if (path.empty())
			return ErrJson("want {\"path\":\"<file.png>\"}");
		int withOverlay = 0;
		if (const JVal* jo = v.find("withOverlay"))
			withOverlay = (jo->type == JVal::BOOL && jo->b) ? 1 : 0;
		int timeoutMs = 3000;
		if (const JVal* jt = v.find("timeoutMs")) if (jt->type == JVal::NUM) timeoutMs = (int)jt->num;

		int w = 0, h = 0;
		const int rc = D2Capture_WriteFramePng(path.c_str(), withOverlay, timeoutMs, &w, &h);
		char b[640];
		if (rc == 1)
		{
			// The path is a WINDOWS path, so it is full of backslashes. Echoing it
			// raw emitted invalid JSON that every strict parser rejects (measured:
			// json.loads raised "Invalid \escape" on a capture that had SUCCEEDED,
			// which reads as a capture failure).
			std::string esc;
			esc.reserve(path.size() + 8);
			for (char c : path)
			{
				if (c == '\\' || c == '"') esc.push_back('\\');
				esc.push_back(c);
			}
			int srcSignedH = 0, bhDst = 0, bhSrc = 0, usedTopDown = -1;
			D2Capture_LastGeometry(&srcSignedH, &bhDst, &bhSrc, &usedTopDown);
			_snprintf_s(b, sizeof(b), _TRUNCATE,
				"{\"ok\":true,\"path\":\"%s\",\"width\":%d,\"height\":%d,\"clean\":%s,"
				"\"srcSignedH\":%d,\"blitHDst\":%d,\"blitHSrc\":%d,\"usedTopDown\":%d}",
				esc.c_str(), w, h, withOverlay ? "false" : "true",
				srcSignedH, bhDst, bhSrc, usedTopDown);
			return std::string(b);
		}
		const char* why =
			rc == -1 ? "timed out waiting for a frame (game minimized or not presenting?)" :
			rc == -2 ? "unsupported surface pixel format (see debugger log for bpp)" :
			           "PNG write failed (bad path or no permission?)";
		_snprintf_s(b, sizeof(b), _TRUNCATE, "{\"ok\":false,\"rc\":%d,\"error\":\"%s\"}", rc, why);
		return std::string(b);
	}

	// ---- VIRTUAL INPUT (D2Debugger.vinput.cpp) --------------------------------------------
	// Unattended gameplay: unlike /showcase/hover-xy (which parks the REAL OS cursor and so
	// fights the operator for the pointer), these drive the game through hooked
	// GetCursorPos/GetAsyncKeyState/GetKeyState. Nothing on the desktop moves.

	// POST /input/mode {"virtual":true|false} -- enable/disable virtual input.
	// While OFF every hook is a straight pass-through, so this is inert by default.
	if (seg[0] == "input" && seg.size() == 2 && seg[1] == "mode" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		const JVal* jv = v.find("virtual");
		if (!jv || jv->type != JVal::BOOL)
			return ErrJson("want {\"virtual\":true|false}");
		D2VInput_SetEnabled(jv->b ? 1 : 0);
		return std::string("{\"ok\":true,\"virtual\":") + (D2VInput_IsEnabled() ? "true" : "false") + "}";
	}

	// GET /input/state -- current virtual mode + virtual cursor position.
	if (seg[0] == "input" && seg.size() == 2 && seg[1] == "state" && method == "GET")
	{
		int x = 0, y = 0; D2VInput_GetScreenPos(&x, &y);
		unsigned long nc = 0, na = 0, nk = 0; D2VInput_GetCounters(&nc, &na, &nk);
		char b[320];
		_snprintf_s(b, sizeof(b), _TRUNCATE,
			"{\"ok\":true,\"virtual\":%s,\"screen\":[%d,%d],\"lbutton\":%s,"
			"\"calls\":{\"GetCursorPos\":%lu,\"GetAsyncKeyState\":%lu,\"GetKeyState\":%lu}}",
			D2VInput_IsEnabled() ? "true" : "false", x, y,
			D2VInput_GetKey(0x01) ? "true" : "false", nc, na, nk);
		return std::string(b);
	}

	// POST /input/move {"x":<gameX>,"y":<gameY>} -- move the VIRTUAL cursor until the game's
	// own mouse view (g_nMouseX/Y) reaches game-space (x,y). Same feedback loop as
	// /showcase/hover-xy -- no DPI or window constants -- but nothing physical moves.
	if (seg[0] == "input" && seg.size() == 2 && seg[1] == "move" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		int x = -1, y = -1;
		if (const JVal* jx = v.find("x")) if (jx->type == JVal::NUM) x = (int)jx->num;
		if (const JVal* jy = v.find("y")) if (jy->type == JVal::NUM) y = (int)jy->num;
		if (x < 0 || y < 0) return ErrJson("want {\"x\":<gameX>,\"y\":<gameY>} (game-space pixels)");
		if (!D2VInput_IsEnabled()) return ErrJson("virtual input is OFF -- POST /input/mode {\"virtual\":true} first");
		std::lock_guard<std::mutex> lk(g_mcpMutex);
		int fx = 0, fy = 0;
		const int rc = D2VInput_MoveToGameXY(x, y, &fx, &fy);
		char b[160];
		if (rc == 1)
		{
			_snprintf_s(b, sizeof(b), _TRUNCATE, "{\"ok\":true,\"gameMouse\":[%d,%d]}", fx, fy);
			return std::string(b);
		}
		_snprintf_s(b, sizeof(b), _TRUNCATE,
			"{\"ok\":false,\"rc\":%d,\"gameMouse\":[%d,%d]}", rc, fx, fy);
		return std::string(b);
	}

	// POST /input/postmove {"x":<clientX>,"y":<clientY>} -- raw WM_MOUSEMOVE probe.
	// Deliberately separate from /input/move so the two candidate mechanisms
	// (GetCursorPos polling vs the message queue) can be tested independently.
	if (seg[0] == "input" && seg.size() == 2 && seg[1] == "postmove" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		int x = -1, y = -1;
		if (const JVal* jx = v.find("x")) if (jx->type == JVal::NUM) x = (int)jx->num;
		if (const JVal* jy = v.find("y")) if (jy->type == JVal::NUM) y = (int)jy->num;
		if (x < 0 || y < 0) return ErrJson("want {\"x\":<clientX>,\"y\":<clientY>}");
		const int ok = D2VInput_PostMouseMove(x, y);
		return std::string("{\"ok\":") + (ok ? "true" : "false") + "}";
	}

	// POST /input/key {"vk":1,"down":true}  -- hold/release a virtual key or mouse button.
	// POST /input/key {"vk":1,"click":true} -- press+release. The press EDGE is latched, so a
	// click that begins and ends between two of the game's polls is still observed exactly once.
	if (seg[0] == "input" && seg.size() == 2 && seg[1] == "key" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		int vk = -1;
		if (const JVal* jk = v.find("vk")) if (jk->type == JVal::NUM) vk = (int)jk->num;
		if (vk < 0 || vk > 255) return ErrJson("want {\"vk\":<0-255>,\"down\"|\"click\":true}");
		if (!D2VInput_IsEnabled()) return ErrJson("virtual input is OFF -- POST /input/mode {\"virtual\":true} first");
		const JVal* jc = v.find("click");
		if (jc && jc->type == JVal::BOOL && jc->b)
		{
			int holdMs = 60;
			if (const JVal* jh = v.find("holdMs")) if (jh->type == JVal::NUM) holdMs = (int)jh->num;
			D2VInput_SetKey(vk, 1);
			Sleep(holdMs);            // span at least one game poll
			D2VInput_SetKey(vk, 0);
			return std::string("{\"ok\":true,\"clicked\":") + std::to_string(vk) + "}";
		}
		const JVal* jd = v.find("down");
		if (!jd || jd->type != JVal::BOOL) return ErrJson("want \"down\":true|false or \"click\":true");
		D2VInput_SetKey(vk, jd->b ? 1 : 0);
		return std::string("{\"ok\":true,\"vk\":") + std::to_string(vk) +
			",\"down\":" + (jd->b ? "true" : "false") + "}";
	}

	// POST /showcase/hover-xy {"x":643,"y":266} -- park the REAL cursor so the game's own mouse view
	// (g_nMouseX/Y) lands on game-space (x,y); feedback-driven, no DPI/window math. With the
	// inventory open, aiming inside an occupied grid cell renders that item's full hover tooltip
	// (screenshot-capturable). Returns the game's final mouse coords.
	if (seg[0] == "showcase" && seg.size() == 2 && seg[1] == "hover-xy" && method == "POST")
	{
		JP jp(body); JVal v = jp.val();
		int x = -1, y = -1;
		if (const JVal* jx = v.find("x")) if (jx->type == JVal::NUM) x = (int)jx->num;
		if (const JVal* jy = v.find("y")) if (jy->type == JVal::NUM) y = (int)jy->num;
		if (x < 0 || y < 0) return ErrJson("want {\"x\":<gameX>,\"y\":<gameY>} (game-space pixels, e.g. 1068x600)");
		std::lock_guard<std::mutex> lk(g_mcpMutex);
		int fx = 0, fy = 0;
		int gs = D2Asset_HoverXY(x, y, &fx, &fy);
		char b[128];
		if (gs == 1)
		{
			_snprintf_s(b, sizeof(b), _TRUNCATE, "{\"ok\":true,\"gameMouse\":[%d,%d]}", fx, fy);
			return std::string(b);
		}
		if (gs == -2) return ErrJson("D2Client.dll not resolved");
		if (gs == -1) return ErrJson("cursor not readable (desktop locked?)");
		_snprintf_s(b, sizeof(b), _TRUNCATE,
			"{\"ok\":false,\"error\":\"did not converge (target off-window?)\",\"gameMouse\":[%d,%d]}", fx, fy);
		return std::string(b);
	}

	// POST /oracle -- GENERAL arbitrary-ABI direct-call oracle (design detail B).
	// Proves ANY D2MOO reimpl live, not just the coord family. Body spec:
	//   { "name":"FN", "callconv":"stdcall|cdecl|fastcall|thiscall",
	//     "ret":"void|i32|u32|ptr",
	//     "args":[ {"id":"a","kind":"i32"}, {"id":"x","kind":"buf","bytes":4} ],
	//     "compare":["ret","x","y"],            // channels compared orig vs reimpl
	//     "vectors":[ {"a":5,"x":100,"y":40}, ... ] }   // per-arg inputs (<=4096)
	// Resolves the RAW original (trampoline if the fn is hooked, else its verified
	// game address) and the provider's reimpl of the same name, marshals each
	// vector through both (fresh scratch buffers per call), and compares.
	if (seg[0] == "oracle" && seg.size() == 1 && method == "POST")
	{
		JP jp(body); JVal spec = jp.val();
		std::string name = spec.s("name");
		if (name.empty()) return ErrJson("missing name");
		int cc = ParseCallConv(spec.s("callconv", "stdcall"));
		if (cc < 0) return ErrJson("bad callconv (cdecl|stdcall|fastcall|thiscall)");
		const std::string ret = spec.s("ret", "void");
		const bool retIsVoid = (ret == "void" || ret.empty());
		const bool ret64 = (ret == "i64" || ret == "u64"); // capture edx:eax
		const uint64_t retMask = RetMask(ret);             // sub-dword returns compare low bits only
		// Marshal onto the GAME THREAD (for fns that need it / touch live state
		// that races the game thread). Both orig+reimpl go through the queue.
		bool onGameThread = false;
		if (const JVal* jg = spec.find("onGameThread"))
			onGameThread = (jg->type == JVal::BOOL && jg->b);
		int gtFail = 0; // 0 ok, -2 timeout, -3 faulted

		// COVERAGE PROBE: byte offsets into the captured live object to read per
		// vector (default dwType at +0), so the coverage analysis can tell which
		// BRANCH each live object exercised (was it 8 distinct units or one type x8?).
		std::vector<int> probeOffsets;
		if (const JVal* jpo = spec.find("probe_offsets"))
			if (jpo->type == JVal::ARR)
				for (const JVal& e : jpo->arr)
					if (e.type == JVal::NUM) probeOffsets.push_back((int)e.num);

		std::vector<OArg> args;
		if (const JVal* ja = spec.find("args"))
			if (ja->type == JVal::ARR)
				for (const JVal& e : ja->arr)
				{
					OArg oa; oa.id = e.s("id");
					const std::string kind = e.s("kind", "i32");
					oa.isSynth = (kind == "synth");   // flat discriminating scratch object (see runOne)
					oa.isSynth2 = (kind == "synth2"); // NESTED: primary-of-pointers -> discriminating secondary
					oa.isBuf = (kind == "buf" || kind == "ptr" || kind == "out" || oa.isSynth || oa.isSynth2);
					oa.isHandle = (kind == "handle"); // live captured game object
					if (const JVal* jb = e.find("bytes")) if (jb->type == JVal::NUM) oa.bytes = (int)jb->num;
					if (oa.bytes < 1) oa.bytes = (oa.isSynth || oa.isSynth2) ? 256 : 4; // room to cover offsets
					// synth/synth2 stay clamped at 256: their discriminating pattern
					// is byte[o] = (o*13 + 0x37) & 0xFF, whose period is exactly 256.
					// Past that the pattern REPEATS, so a reimpl reading offset o+256
					// instead of o would read the same byte and falsely match --
					// destroying the very property synth exists to provide. Plain
					// out-buffers carry no such constraint, so a mutator writing a
					// larger struct is allowed the room it needs.
					if (oa.isSynth || oa.isSynth2) { if (oa.bytes > 256) oa.bytes = 256; }
					else if (oa.bytes > 4096) oa.bytes = 4096;
					// optional type-gate patches (satisfy `field==imm` preconditions on synth objs)
					if (const JVal* jg = e.find("gates"))
						if (jg->type == JVal::ARR)
							for (const JVal& g : jg->arr)
							{
								OGate og;
								if (const JVal* jd = g.find("depth")) if (jd->type == JVal::NUM) og.depth = (int)jd->num;
								if (const JVal* jo = g.find("off"))   if (jo->type == JVal::NUM) og.off = (int)jo->num;
								if (const JVal* ji = g.find("imm"))   if (ji->type == JVal::NUM) og.imm = (uint32_t)ji->num;
								if (const JVal* jw = g.find("w"))     if (jw->type == JVal::NUM) og.w = (int)jw->num;
								if (og.w < 1 || og.w > 4) og.w = 4;
								oa.gates.push_back(og);
							}
					args.push_back(oa);
				}
		if (args.size() > 8) return ErrJson("too many args (max 8 slots)");

		std::vector<std::string> cmp;
		if (const JVal* jc = spec.find("compare"))
			if (jc->type == JVal::ARR)
				for (const JVal& e : jc->arr) if (e.type == JVal::STR) cmp.push_back(e.str);
		bool cmpRet = false;
		for (auto& c : cmp) if (c == "ret") cmpRet = true;

		// A buffer compare channel on the game-thread path would compare
		// empty-vs-empty and PASS for every vector -- a silent false proof, and
		// the worst possible failure for a conformance oracle. D2Gt_Call2 does
		// not read buffers back, so reject the combination loudly instead.
		if (onGameThread)
			for (auto& c : cmp)
			{
				if (c == "ret") continue;
				for (size_t k = 0; k < args.size(); ++k)
					if (args[k].isBuf && args[k].id == c)
					{
						const std::string msg = "buffer compare channel '" + c +
							"' is not supported with onGameThread (buffers are not read "
							"back on the game-thread path; the comparison would pass vacuously)";
						return ErrJson(msg.c_str());
					}
			}

		// Register-explicit ORIGINAL call (custom ABI). orig_regs maps a GP
		// register -> arg id; that arg's value (scalar) or pointer (buf) is placed
		// in that register before calling the original. The reimpl is still called
		// via the standard marshaller (we write it in a normal convention), so the
		// two agree on logical inputs without a hand-asm reimpl.
		int regArgIdx[6] = { -1, -1, -1, -1, -1, -1 };
		bool origUsesRegs = false;
		if (const JVal* jr = spec.find("orig_regs"))
			if (jr->type == JVal::OBJ)
				for (auto& kv : jr->obj)
				{
					const int ri = RegIndex(kv.first);
					if (ri < 0 || kv.second.type != JVal::STR) continue;
					for (size_t k = 0; k < args.size(); ++k)
						if (args[k].id == kv.second.str) { regArgIdx[ri] = (int)k; origUsesRegs = true; break; }
				}

		const JVal* vecs = spec.find("vectors");
		if (!vecs || vecs->type != JVal::ARR || vecs->arr.empty())
			return ErrJson("missing vectors");
		if (vecs->arr.size() > 4096) return ErrJson("too many vectors (max 4096)");

		// Resolve the RAW original. Precedence: explicit PD2 identity from the
		// spec (what fun-doc knows from Ghidra) wins over name lookup, because
		// D2MOO's names mostly DON'T match PD2's scrambled export/Ghidra names
		// (ORDINAL_RECONCILIATION.md) -- only a verified identity is safe:
		//   "module"+"rva": PREFERRED -- resolved against the RUNTIME base, so it
		//                   is correct whether or not the module got its preferred
		//                   base (see ResolveModuleRva; this is the D2Client fix).
		//   "offset":       D2Common-relative (added to base 0x6fd50000) -- legacy,
		//                   only sound because D2Common does load there.
		//   "addr":         absolute game address -- legacy, and WRONG for any
		//                   relocated module. Kept for back-compat; the
		//                   IsCallableAddress gate below catches its bad cases.
		// Fallback: dispatcher trampoline (if hooked) else name via the
		// verified-address resolver.
		const unsigned int kD2CommonBase = 0x6fd50000u;
		const int di = DispatcherIndexForName(name.c_str());
		void* orig = nullptr;
		const char* origVia = "";
		const std::string mod = spec.s("module");
		if (!mod.empty())
		{
			if (const JVal* jr = spec.find("rva"))
				if (jr->type == JVal::NUM)
				{
					if (!GetModuleHandleA(mod.c_str()))
						return ErrJson((std::string("module not loaded in the game process: ") + mod
							+ " -- bad-target (nothing was called; this is NOT a verdict about the function)").c_str());
					orig = ResolveModuleRva(mod.c_str(), (unsigned int)jr->num);
					origVia = "module+rva";
				}
		}
		if (!orig) if (const JVal* jo = spec.find("offset"))
		{ if (jo->type == JVal::NUM) { orig = (void*)(uintptr_t)(kD2CommonBase + (unsigned int)jo->num); origVia = "offset"; } }
		if (!orig) if (const JVal* ja = spec.find("addr"))
		{ if (ja->type == JVal::NUM) { orig = (void*)(uintptr_t)(unsigned int)ja->num; origVia = "addr"; } }
		if (!orig)
		{
			orig = (di >= 0 && g_bridge.getTrampoline) ? BrTrampoline(di)
				: (g_bridge.resolveGameFn ? g_bridge.resolveGameFn(name.c_str()) : nullptr);
			origVia = "name";
		}
		void* reimpl = g_provider ? (void*)MemoryGetProcAddress(g_provider, name.c_str()) : nullptr;
		if (!reimpl && di >= 0 && g_bridge.getReimpl) reimpl = BrReimpl(di);
		if (!orig)   return ErrJson("original not resolved (supply module+rva, offset/addr, or a name in the verified-address table)");
		if (!reimpl) return ErrJson("reimpl not found (provider must export this name)");

		// BAD-TARGET GATE. Refuse to `call` an address that is not mapped
		// executable. Without this the call faults, SEH catches it, and the caller
		// receives the same "handler-exception" a genuinely wrong ABI produces --
		// so a base/rebasing mistake gets filed as a verdict about the reimpl. The
		// "bad-target" token below is what fun-doc keys on to treat this as
		// ENVIRONMENTAL and re-queue the function instead of retiring it.
		if (!IsCallableAddress(orig))
		{
			char buf[320];
			_snprintf_s(buf, sizeof(buf), _TRUNCATE,
				"bad-target: original for %s resolved via %s to 0x%08X, which is not mapped "
				"executable in the game process (nothing was called; this is NOT a verdict "
				"about the function -- check the module's RUNTIME base, not Ghidra's image base)",
				name.c_str(), origVia, (unsigned int)(uintptr_t)orig);
			return ErrJson(buf);
		}
		if (!IsCallableAddress(reimpl))
			return ErrJson("bad-target: reimpl address is not mapped executable (provider load is broken)");

		// Live-handle guard: a "handle" arg needs a captured live object to exist.
		bool needsHandle = false, anyBuf = false;
		for (const OArg& oa : args) { if (oa.isHandle) needsHandle = true; if (oa.isBuf) anyBuf = true; }
		if (needsHandle && !D2Capture_LastUnit())
			return ErrJson("no live game-object handle captured yet (be in-game; UNIT_GetMode must have fired)");
		if (needsHandle && probeOffsets.empty())
			probeOffsets.push_back(0);   // default: probe dwType (+0) for branch coverage
		// Atomic orig+reimpl is eligible when both run on the game thread with the
		// SAME standard-convention slots + no buffer readback (the live-object getter
		// case). Register-explicit originals or buffer args keep the two-call path.
		const bool atomicPair = onGameThread && !origUsesRegs && !anyBuf;
		// Distinct captured objects (one per unit type). Running each vector against
		// a DIFFERENT type makes one proof exercise every type-dispatched branch --
		// closing the "all vectors saw the same unit standing still" coverage gap.
		void* distinctObjs[8]; int nDistinct = 0;
		if (needsHandle) nDistinct = D2Capture_FillDistinct(distinctObjs, 8);

		// Snapshot of the captured live handle for the CURRENT vector: set once
		// per vector (below) so the original and the reimpl are called with the
		// EXACT SAME pointer. Reading D2Capture_LastUnit() separately inside each
		// runOne let the game thread swap the captured object BETWEEN the two
		// calls -> a false mismatch on an otherwise-correct reimpl.
		uint32_t handleSnap = 0;

		// Run one target on one vector -> (ret, per-buffer readback values).
		auto runOne = [&](void* fn, const JVal& vec, bool useRegs, uint64_t& retOut, std::vector<std::vector<uint8_t>>& bufOut)
		{
			std::vector<std::vector<uint8_t>> bufs(args.size());
			std::vector<uint32_t> slots(args.size());
			for (size_t k = 0; k < args.size(); ++k)
			{
				const JVal* v = vec.find(args[k].id.c_str());
				long long iv = (v && v->type == JVal::NUM) ? v->num : 0;
				if (args[k].isHandle)
				{
					slots[k] = handleSnap; // per-vector snapshot: orig & reimpl get the SAME live object
				}
				else if (args[k].isBuf)
				{
					bufs[k].assign(args[k].bytes, 0);
					if (args[k].isSynth)
					{
						// DISCRIMINATING synthetic object: byte[o] = (o*13 + 0x37) mod 256.
						// 13 is coprime to 256, so within a 256-byte buffer EVERY offset gets
						// a UNIQUE byte -> byte/word/dword reads at different offsets all yield
						// different values. A getter that reads a fixed field offset thus
						// returns a value UNIQUE to that offset, so a wrong-offset reimpl
						// MISMATCHES the original -- killing the degenerate all-zeros false
						// positive that idle-town live captures produce. SAFE for FLAT getters
						// only (a single fixed-offset read, no sub-pointer deref) -- the caller
						// must gate on that (a synth byte is not a valid pointer to deref).
						for (int o = 0; o < args[k].bytes; ++o)
							bufs[k][o] = (uint8_t)((o * 13 + 0x37) & 0xFF);
					}
					else if (args[k].isSynth2)
					{
						// NESTED discriminating object for a 2-LEVEL getter (read a pointer at
						// O1, deref, read the field at O2). Layout: [0..bytes) PRIMARY = an array
						// of pointers, every one pointing to [bytes..bytes+256) SECONDARY, which
						// holds the byte[o]=(o*13+0x37) discriminating pattern. So the getter
						// returns pattern(O2) -> a wrong FIELD offset O2 mismatches the original.
						// (The substruct-pointer offset O1 is disasm-derived by the mechanical
						// translator, so it is correct by construction; every primary slot points
						// to the same secondary, so any O1 safely derefs.) SAFE for 2-deref getters.
						const int PRIMARY = args[k].bytes;   // pointer table
						const int SECOND = 256;              // discriminating field region
						bufs[k].assign(PRIMARY + SECOND, 0);
						uint8_t* sec = &bufs[k][PRIMARY];
						for (int o = 0; o < SECOND; ++o) sec[o] = (uint8_t)((o * 13 + 0x37) & 0xFF);
						const uint32_t secPtr = (uint32_t)(uintptr_t)sec;
						for (int o = 0; o + 4 <= PRIMARY; o += 4)
							*reinterpret_cast<uint32_t*>(&bufs[k][o]) = secPtr;
					}
					else
					{
						int w = args[k].bytes < 8 ? args[k].bytes : 8;
						for (int b = 0; b < w; ++b) bufs[k][b] = (uint8_t)((iv >> (8 * b)) & 0xFF);
					}
					// TYPE-GATE patches: overwrite precondition fields so a gated getter
					// (if pUnit->dwType != 4 return 0) takes its SUCCESS path and reads the
					// discriminating field. Without this both orig & reimpl bail the gate to
					// the default return -> a DEGENERATE (false-strong) match. depth 0 patches
					// the primary/flat buffer; depth 1 patches the synth2 secondary region.
					for (const OGate& g : args[k].gates)
					{
						size_t gbase = (args[k].isSynth2 && g.depth == 1) ? (size_t)args[k].bytes : 0;
						size_t at = gbase + (size_t)g.off;
						if (g.off >= 0 && at + (size_t)g.w <= bufs[k].size())
							for (int b = 0; b < g.w; ++b)
								bufs[k][at + b] = (uint8_t)((g.imm >> (8 * b)) & 0xFF);
					}
					slots[k] = (uint32_t)(uintptr_t)bufs[k].data();
				}
				else slots[k] = (uint32_t)iv;
			}
			if (useRegs)
			{
				// place each mapped arg's slot into its GP register; call; capture.
				uint32_t io[6] = { 0, 0, 0, 0, 0, 0 };
				for (int r = 0; r < 6; ++r)
					if (regArgIdx[r] >= 0) io[r] = slots[regArgIdx[r]];
				D2Oracle_CallRegs(fn, io);
				retOut = ret64 ? ((uint64_t)io[0] | ((uint64_t)io[2] << 32)) : io[0]; // EAX (:EDX)
			}
			else if (onGameThread)
			{
				uint64_t r = 0;
				int gs = D2Gt_Call(fn, cc, slots.data(), (int)args.size(), ret64 ? 1 : 0, &r, 2500);
				if (gs <= 0) gtFail = (gs == 0 ? -2 : -3);
				retOut = r;
			}
			else
			{
				retOut = ret64 ? D2Oracle_Call64(fn, cc, slots.data(), (int)args.size())
				               : D2Oracle_Call(fn, cc, slots.data(), (int)args.size());
			}
			// FULL-WIDTH readback (2026-07-30). This used to pack only the FIRST 8
			// BYTES of the buffer into a uint64 and compare that, so any write at
			// offset >= 8 was invisible to the comparison. That single line is why
			// ~830 void-return mutators on D2Client alone were classified
			// "no comparable output" and dead-ended in stateful_skip: the channel
			// existed, it was just 8 bytes wide. Copy the whole comparable region
			// instead; the caller memcmps it.
			//
			// WHICH region is comparable depends on the kind:
			//   synth2 -> ONLY the SECONDARY [bytes, bytes+256). The PRIMARY region
			//     is a table of raw pointers into this call's own scratch allocation,
			//     so its bytes differ between the orig and reimpl calls BY
			//     CONSTRUCTION -- comparing it would mismatch 100% of the time. The
			//     secondary is where a write-through-the-deref actually lands, which
			//     is exactly what a 2-level mutator should be judged on.
			//   everything else -> the flat [0, bytes).
			bufOut.assign(args.size(), std::vector<uint8_t>());
			for (size_t k = 0; k < args.size(); ++k) if (args[k].isBuf)
			{
				const size_t off = args[k].isSynth2 ? (size_t)args[k].bytes : 0;
				const size_t n = args[k].isSynth2 ? (size_t)256 : (size_t)args[k].bytes;
				if (off + n <= bufs[k].size())
					bufOut[k].assign(bufs[k].begin() + off, bufs[k].begin() + off + n);
			}
		};

		int matches = 0;
		std::string results = "[";
		for (size_t vi = 0; vi < vecs->arr.size(); ++vi)
		{
			const JVal& vec = vecs->arr[vi];
			uint64_t retO = 0, retR = 0; std::vector<std::vector<uint8_t>> bO, bR;
			// Per vector: a DIFFERENT captured type when we have several (round-robin
			// -> branch diversity), else the last captured object. Snapshotted ONCE so
			// both orig+reimpl see the same object.
			handleSnap = (nDistinct > 0)
				? (uint32_t)(uintptr_t)distinctObjs[vi % nDistinct]
				: (uint32_t)(uintptr_t)D2Capture_LastUnit();
			// COVERAGE PROBE: read the captured object's dispatch field(s) for THIS
			// vector's object, so we know which branch it exercised.
			std::string probeJson;
			if (!probeOffsets.empty() && handleSnap)
			{
				probeJson = ",\"probe\":[";
				for (size_t pi = 0; pi < probeOffsets.size(); ++pi)
				{
					uint32_t pv = SafeReadU32((const uint8_t*)(uintptr_t)handleSnap + probeOffsets[pi]);
					char pb[16]; _snprintf_s(pb, sizeof(pb), _TRUNCATE, "%s%u", pi ? "," : "", pv);
					probeJson += pb;
				}
				probeJson += "]";
			}
			if (atomicPair)
			{
				// Marshal the shared slots (handle -> snapshot, else scalar) and run
				// BOTH functions in ONE game-thread pump -- no frame between them, so
				// a volatile live-object field reads identically for orig and reimpl.
				std::vector<uint32_t> slots(args.size());
				for (size_t k = 0; k < args.size(); ++k)
				{
					if (args[k].isHandle) { slots[k] = handleSnap; continue; }
					const JVal* v = vec.find(args[k].id.c_str());
					slots[k] = (uint32_t)((v && v->type == JVal::NUM) ? v->num : 0);
				}
				int gs = D2Gt_Call2(orig, reimpl, cc, slots.data(), (int)args.size(),
					ret64 ? 1 : 0, &retO, &retR, 2500);
				if (gs <= 0) gtFail = (gs == 0 ? -2 : -3);
				// NOTE: the game-thread path does not read buffers back. Buffer
				// compare channels are rejected up front when onGameThread is set
				// (see the guard after `cmp` is parsed) -- otherwise every buf
				// channel would compare empty-vs-empty and PASS vacuously.
				bO.assign(args.size(), std::vector<uint8_t>());
				bR.assign(args.size(), std::vector<uint8_t>());
			}
			else
			{
				runOne(orig, vec, origUsesRegs, retO, bO);  // original: custom register ABI if orig_regs given
				runOne(reimpl, vec, false, retR, bR);        // reimpl: standard convention
			}

			bool m = true;
			if (cmpRet && !retIsVoid && (retO & retMask) != (retR & retMask)) m = false;
			std::string bufsJson;
			for (auto& c : cmp)
			{
				if (c == "ret") continue;
				for (size_t k = 0; k < args.size(); ++k)
					if (args[k].isBuf && args[k].id == c)
					{
						// Full-region compare. `o`/`r` keep their original meaning
						// (the low 8 bytes, packed LE) so existing consumers --
						// prove_candidate.py's mismatch print, adversarial_reproof's
						// pass-through -- keep working; `n` and `off` are additive.
						const std::vector<uint8_t>& vo = bO[k];
						const std::vector<uint8_t>& vr = bR[k];
						int diffOff = -1;
						if (vo.size() != vr.size()) diffOff = 0;
						else for (size_t b2 = 0; b2 < vo.size(); ++b2)
							if (vo[b2] != vr[b2]) { diffOff = (int)b2; break; }
						if (diffOff >= 0) m = false;
						auto low8 = [](const std::vector<uint8_t>& v) {
							uint64_t val = 0; size_t w = v.size() < 8 ? v.size() : 8;
							for (size_t b3 = 0; b3 < w; ++b3) val |= (uint64_t)v[b3] << (8 * b3);
							return val;
						};
						char b[224];
						_snprintf_s(b, sizeof(b), _TRUNCATE,
							"%s\"%s\":{\"o\":%llu,\"r\":%llu,\"n\":%d,\"off\":%d}",
							bufsJson.empty() ? "" : ",", c.c_str(),
							(unsigned long long)low8(vo), (unsigned long long)low8(vr),
							(int)vo.size(), diffOff);
						bufsJson += b;
					}
			}
			if (m) ++matches;
			char head[224];
			_snprintf_s(head, sizeof(head), _TRUNCATE,
				"%s{\"match\":%s,\"ret\":{\"o\":%llu,\"r\":%llu},\"bufs\":{",
				vi ? "," : "", m ? "true" : "false",
				(unsigned long long)retO, (unsigned long long)retR);
			results += head; results += bufsJson; results += "}";
			results += probeJson; results += "}";
		}
		results += "]";

		if (gtFail == -2) return ErrJson("game-thread call timed out (be in-world; the capture/pump hook must be firing)");
		if (gtFail == -3) return ErrJson("game-thread call FAULTED (SEH-caught) -- bad handle/args?");

		const int cnt = (int)vecs->arr.size();
		return "{\"ok\":true,\"name\":" + JStr(name) +
			",\"count\":" + std::to_string(cnt) +
			",\"matches\":" + std::to_string(matches) +
			",\"mismatches\":" + std::to_string(cnt - matches) +
			",\"allMatch\":" + (matches == cnt ? "true" : "false") +
			",\"results\":" + results + "}";
	}

	return ErrJson("unknown route");
}

void D2DebugLiveDispatch()
{
	static RegistryCache cache;
	if (!cache.loaded)
		LoadRegistry(cache);
	D2Prof_EnsureTable();
	ResolveBridge();

	if (!ImGui::Begin("Live Dispatch Registry"))
	{
		ImGui::End();
		return;
	}

	ImGui::TextWrapped(
		"Unified function browser: every D2Common export grouped by subsystem, with "
		"LIVE hit counts. Click a subsystem's [instrument] button to count-hook JUST "
		"that subsystem (Original passthrough, no behavior change). A blind mass-hook "
		"of all 1,172 exports originally crashed PD2 -- NOT from the count, but from "
		"two now-fixed bugs: aliased ordinals (many exports -> the same address; "
		"hooking it twice double-patched the prologue) and DATA exports (e.g. "
		"g_pDataTables, whose bytes got overwritten with a jump). The profiler now "
		"dedups by address + gates non-code exports (see the dedup/unhookable counter "
		"below), so this is safe; per-subsystem just keeps the blast radius small. "
		"Functions D2MOO has a dispatcher for also show a mode toggle "
		"(Original / Reimpl / Shadow) + divergence.");

	// --- Profiler status ---
	ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.35f, 1.0f),
		"Profiler: %d hooked, %d skipped (dedup/unhookable)", D2Prof_Hooked(), D2Prof_Skipped());
	ImGui::SameLine();
	// One-click whole-engine instrumentation: loops every subsystem, one
	// transaction each (same safe path as the per-subsystem [instrument]
	// buttons; idempotent, so it just fills in whatever isn't hooked yet).
	if (ImGui::Button("Instrument all"))
		D2Prof_InstallAll();
	ImGui::SameLine();
	if (ImGui::Button("Reset counters"))
		D2Prof_Reset();
	ImGui::SameLine();
	if (g_bridge.available)
		ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.35f, 1.0f), "| Bridge: %d dispatchers", BrCount());
	else
		ImGui::TextColored(ImVec4(0.90f, 0.55f, 0.25f, 1.0f), "| Bridge: none");

	// --- Global dispatch controls ---
	if (g_bridge.available)
	{
		const int nb = BrCount();
		if (ImGui::Button("Shadow all dispatchers")) { for (int i = 0; i < nb; ++i) BrSetMode(i, 2); }
		ImGui::SameLine();
		if (ImGui::Button("Original all dispatchers")) { for (int i = 0; i < nb; ++i) BrSetMode(i, 0); }
		ImGui::SameLine();
		if (ImGui::Button("Reload registry")) cache.loaded = false;

		// WS-1 hot-reload: (re)load the reimpl-provider DLL live -- add/replace
		// equivalents without restarting PD2.
		if (g_bridge.quiesce && g_bridge.setReimpl)
		{
			if (ImGui::Button("Reload reimpl provider"))
				ReloadProvider();
			ImGui::SameLine();
			ImGui::TextColored(g_provider ? ImVec4(0.35f, 0.85f, 0.35f, 1.0f) : ImVec4(0.75f, 0.75f, 0.75f, 1.0f),
				"%s", g_providerStatus);
		}
	}

	static char filter[64] = "";
	ImGui::SetNextItemWidth(220);
	ImGui::InputTextWithHint("##filter", "filter by name...", filter, sizeof(filter));

	// registry.json ordinal -> proof_status (badge overlay).
	std::map<long long, std::string> proofByOrd;
	for (const RegistryRow& r : cache.rows)
		if (r.has_ordinal)
			proofByOrd[r.real_ordinal] = r.proof_status;

	// Effective hits: dispatcher-owned funcs are hooked by the dispatcher (the
	// profiler skips them), so their count comes from the bridge; everything else
	// from the profiler's own counter.
	auto effHits = [&](int i) -> unsigned long long
	{
		const int bi = BridgeIndexForOffset(D2Prof_Offset(i));
		if (bi >= 0 && g_bridge.available) return BrHits(bi);
		return D2Prof_Hits(i);
	};

	// Group by subsystem.
	struct Cat { std::vector<int> idx; unsigned long long hits = 0; };
	std::map<std::string, Cat> cats;
	const int n = D2Prof_Count();
	unsigned long long grand = 0;
	for (int i = 0; i < n; ++i)
	{
		Cat& c = cats[D2Prof_Category(i)];
		c.idx.push_back(i);
		const unsigned long long h = effHits(i);
		c.hits += h;
		grand += h;
	}
	ImGui::Text("Total: %llu hits / %d functions / %d subsystems",
		grand, n, (int)cats.size());

	std::vector<std::pair<std::string, Cat*>> ordered;
	for (auto& kv : cats) ordered.emplace_back(kv.first, &kv.second);
	std::sort(ordered.begin(), ordered.end(),
		[](const std::pair<std::string, Cat*>& a, const std::pair<std::string, Cat*>& b)
		{ return a.second->hits > b.second->hits; });

	static const char* kModeItems[] = { "Original", "Reimpl", "Shadow" };

	if (ImGui::BeginChild("functree", ImVec2(0, 470), true))
	{
		for (auto& oc : ordered)
		{
			Cat& c = *oc.second;
			ImGui::PushID(oc.first.c_str());
			char hdr[160];
			_snprintf_s(hdr, sizeof(hdr), _TRUNCATE, "%s  (%llu hits, %d fns)###node",
				oc.first.c_str(), c.hits, (int)c.idx.size());

			ImGui::PushStyleColor(ImGuiCol_Text, c.hits > 0
				? ImVec4(0.55f, 0.90f, 0.55f, 1.0f) : ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
			const bool open = ImGui::TreeNode(hdr);
			ImGui::PopStyleColor();

			// Per-subsystem instrument control (the SAFE, small-batch install).
			ImGui::SameLine();
			if (D2Prof_IsCategoryInstalled(oc.first.c_str()))
				ImGui::TextDisabled("[instrumented]");
			else if (ImGui::SmallButton("instrument"))
				D2Prof_InstallCategory(oc.first.c_str());

			if (!open)
			{
				ImGui::PopID();
				continue;
			}

			std::vector<int> kids = c.idx;
			std::sort(kids.begin(), kids.end(),
				[&](int a, int b) { return effHits(a) > effHits(b); });

			for (int i : kids)
			{
				if (filter[0] && !strstr(D2Prof_Name(i), filter))
					continue;
				ImGui::PushID(i);
				const unsigned long long h = effHits(i);
				const int bi = BridgeIndexForOffset(D2Prof_Offset(i));
				const bool hasEquiv = (bi >= 0 && g_bridge.available);

				// Mode control FIRST (fixed position -> aligns on every row). A
				// function with a one-for-one equivalent gets the full editable
				// Original/Reimpl/Shadow combo; every other function shows a
				// LOCKED "Original" (it has no equivalent to switch to -- it's
				// always running the game's original code).
				ImGui::SetNextItemWidth(84);
				if (hasEquiv)
				{
					int mode = BrGetMode(bi);
					if (ImGui::Combo("##m", &mode, kModeItems, IM_ARRAYSIZE(kModeItems)))
						BrSetMode(bi, mode);
				}
				else
				{
					int only = 0;
					ImGui::BeginDisabled();
					ImGui::Combo("##m", &only, "Original\0");
					ImGui::EndDisabled();
				}

				// Divergence (only meaningful for equivalents).
				ImGui::SameLine();
				if (hasEquiv)
				{
					const unsigned long long d = BrDiv(bi);
					if (d > 0) ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "div:%-6llu", d);
					else ImGui::TextDisabled("div:0    ");
				}
				else
				{
					ImGui::TextDisabled("         ");
				}

				// Ordinal / name.
				ImGui::SameLine();
				ImGui::TextColored(h > 0 ? ImVec4(0.88f, 0.88f, 0.88f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
					"@%-6d %-46s", D2Prof_Ordinal(i), D2Prof_Name(i));

				// Hits column -- disambiguated so a "0" is interpretable:
				//   subsystem not instrumented -> "--"      (not counting yet)
				//   instrumented, unhookable    -> "no-hook" (dedup/alias or Detours refused)
				//   instrumented, 0 calls       -> "0"       (hooked, genuinely not called)
				//   >0                          -> the count (green)
				const bool catInstalled = D2Prof_IsCategoryInstalled(D2Prof_Category(i));
				ImGui::SameLine();
				if (hasEquiv || D2Prof_IsHooked(i))
				{
					if (h > 0) ImGui::TextColored(ImVec4(0.55f, 0.90f, 0.55f, 1.0f), "%10llu", h);
					else ImGui::TextDisabled("%10llu", 0ull);
				}
				else if (catInstalled)
				{
					ImGui::TextColored(ImVec4(0.80f, 0.55f, 0.30f, 1.0f), "%10s", "no-hook");
				}
				else
				{
					ImGui::TextDisabled("%10s", "--");
				}

				// Proof badge from registry.json (by ordinal).
				auto pit = proofByOrd.find(D2Prof_Ordinal(i));
				if (pit != proofByOrd.end())
				{
					ImGui::SameLine();
					ImGui::TextColored(ColorForProofStatus(pit->second), "[%s]", pit->second.c_str());
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
			ImGui::PopID(); // matches PushID(category)
		}

		// --- Dispatchers with NO profiler row -------------------------------
		// The tree above is built from D2Prof_*, which enumerates D2COMMON
		// exports only. Once multi-bridge added other patch modules, their
		// dispatchers became invisible here -- D2Client's GetItemQualityStringId
		// was live, shadowing and promoted to CONF_BATTLETESTED while this panel
		// showed nothing at all. Anything the profiler cannot account for is
		// listed here so the panel never silently omits a live dispatcher.
		{
			std::map<uint32_t, int> bridgeByOffset;
			for (int bi = 0; bi < BrCount(); ++bi)
				bridgeByOffset[BrOffset(bi)] = bi;
			std::vector<bool> claimed(BrCount(), false);
			for (int i = 0, n = D2Prof_Count(); i < n; ++i)
			{
				auto it = bridgeByOffset.find(D2Prof_Offset(i));
				if (it != bridgeByOffset.end()
					&& _stricmp(BrModule(it->second), "D2Common.dll") == 0)
					claimed[it->second] = true;
			}

			std::vector<int> orphans;
			for (int bi = 0; bi < BrCount(); ++bi)
				if (!claimed[bi]) orphans.push_back(bi);

			if (!orphans.empty())
			{
				ImGui::PushID("unprofiled");
				char hdr[96];
				_snprintf_s(hdr, sizeof(hdr), _TRUNCATE,
					"Other patch modules  (%d dispatchers, no profiler rows)###unprof",
					(int)orphans.size());
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.75f, 0.95f, 1.0f));
				const bool open = ImGui::TreeNode(hdr);
				ImGui::PopStyleColor();
				if (open)
				{
					for (int bi : orphans)
					{
						ImGui::PushID(bi);
						ImGui::SetNextItemWidth(84);
						int mode = BrGetMode(bi);
						if (ImGui::Combo("##m", &mode, kModeItems, IM_ARRAYSIZE(kModeItems)))
							BrSetMode(bi, mode);

						ImGui::SameLine();
						const unsigned long long dv = BrDiv(bi);
						if (dv > 0) ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "div:%-6llu", dv);
						else ImGui::TextDisabled("div:0    ");

						// A null trampoline means ApplyPatchAction never installed
						// -- the difference between "never hooked" and "never
						// called", which is otherwise indistinguishable from 0 hits.
						ImGui::SameLine();
						if (!BrTrampoline(bi))
							ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.25f, 1.0f), "%-9s", "NOHOOK");
						else
							ImGui::TextDisabled("%-9s", "");

						ImGui::SameLine();
						const unsigned long long h = BrHits(bi);
						ImGui::TextColored(h > 0 ? ImVec4(0.88f, 0.88f, 0.88f, 1.0f)
											     : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
							"%-16s %-40s", BrModule(bi), BrName(bi));

						ImGui::SameLine();
						if (h > 0) ImGui::TextColored(ImVec4(0.55f, 0.90f, 0.55f, 1.0f), "%10llu", h);
						else ImGui::TextDisabled("%10llu", 0ull);

						// Input diversity -- the promoter's other gate.
						if (BrHasDiversity(bi))
						{
							ImGui::SameLine();
							ImGui::TextDisabled("  d:%llu", (unsigned long long)BrDistinct(bi));
						}
						ImGui::PopID();
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
		}
	}
	ImGui::EndChild();

	ImGui::End();
}
