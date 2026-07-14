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
	};
	Bridge g_bridge;

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
		const int n = g_bridge.getCount();
		std::vector<int> prevModes(n);
		for (int i = 0; i < n; ++i)
			prevModes[i] = g_bridge.getMode(i);

		if (g_bridge.quiesce() != 1)
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
				for (int i = 0; i < n; ++i) g_bridge.setMode(i, prevModes[i]);
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
			for (int i = 0; i < n; ++i) g_bridge.setMode(i, prevModes[i]);
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
			void* fn = (void*)MemoryGetProcAddress(g_provider, g_bridge.getName(i));
			if (fn) { g_bridge.setReimpl(i, fn); ++bound; }
		}
		for (int i = 0; i < n; ++i)
			g_bridge.setMode(i, prevModes[i]);

		_snprintf_s(g_providerStatus, sizeof(g_providerStatus), _TRUNCATE,
			"provider: mem-loaded #%d, %d/%d bound (modes restored)", g_reloadSeq, bound, n);
	}

	// offset -> bridge index, rebuilt from the bridge each frame it's available.
	// DATA-DRIVEN: any dispatcher added to the patch bridge auto-appears here, so
	// newly-ported equivalents become shadow-selectable with no UI code change.
	int BridgeIndexForOffset(uint32_t off)
	{
		if (!g_bridge.available || !g_bridge.getOffset)
			return -1;
		const int n = g_bridge.getCount();
		for (int i = 0; i < n; ++i)
			if (g_bridge.getOffset(i) == off)
				return i;
		return -1;
	}

	void ResolveBridge()
	{
		if (g_bridge.resolved)
			return;
		g_bridge.resolved = true;

		// The real game's D2Common.dll and D2MOO's patch copy share the base name
		// "D2Common.dll", so GetModuleHandle is ambiguous. Disambiguate by the ONE
		// export only the patch has: whichever loaded module answers
		// D2MOO_LiveDispatch_GetCount is the patch copy.
		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
		if (snap == INVALID_HANDLE_VALUE)
			return;
		MODULEENTRY32W me{};
		me.dwSize = sizeof(me);
		if (Module32FirstW(snap, &me))
		{
			do
			{
				auto f = (GetCountFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetCount");
				if (f)
				{
					g_bridge.getCount = f;
					g_bridge.getName = (GetNameFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetName");
					g_bridge.getMode = (GetModeFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetMode");
					g_bridge.setMode = (SetModeFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_SetMode");
					g_bridge.getHits = (GetU64Fn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetHits");
					g_bridge.getDiv = (GetU64Fn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetDivergences");
					g_bridge.getOffset = (GetOffsetFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetOffset");
					g_bridge.setReimpl = (SetReimplFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_SetReimpl");
					g_bridge.quiesce = (QuiesceFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_QuiesceForReload");
					g_bridge.resolveGameFn = (ResolveGameFn)GetProcAddress(me.hModule, "D2MOO_ResolveGameFn");
					g_bridge.getTrampoline = (GetPtrFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetTrampoline");
					g_bridge.getReimpl = (GetPtrFn)GetProcAddress(me.hModule, "D2MOO_LiveDispatch_GetReimpl");
					g_bridge.available = g_bridge.getName && g_bridge.getMode && g_bridge.setMode &&
						g_bridge.getHits && g_bridge.getDiv && g_bridge.getOffset;
					break;
				}
			} while (Module32NextW(snap, &me));
		}
		CloseHandle(snap);
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
		if (bi >= 0 && g_bridge.available) return g_bridge.getHits(bi);
		return D2Prof_Hits(i);
	}

	std::string DispatcherJson(int i)
	{
		std::string name = g_bridge.getName ? g_bridge.getName(i) : "";
		char buf[512];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"{\"index\":%d,\"name\":%s,\"offset\":%u,\"mode\":%d,\"modeName\":\"%s\",\"hits\":%llu,\"divergences\":%llu}",
			i, JStr(name).c_str(), g_bridge.getOffset(i), g_bridge.getMode(i), ModeName(g_bridge.getMode(i)),
			(unsigned long long)g_bridge.getHits(i), (unsigned long long)g_bridge.getDiv(i));
		return buf;
	}

	std::string ErrJson(const char* msg)
	{
		std::string o = "{\"ok\":false,\"error\":"; o += JStr(msg); o += "}"; return o;
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
		const int n = g_bridge.getCount();
		for (int i = 0; i < n; ++i)
		{
			const char* nm = g_bridge.getName(i);
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
		char buf[1024];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"{\"ok\":true,\"bridge\":%s,\"dispatchers\":%d,\"provider\":%s,\"reloadSeq\":%d,"
			"\"profilerHooked\":%d,\"profilerSkipped\":%d,\"functions\":%d,"
			"\"capturedHandle\":\"0x%08x\",\"captureCount\":%u,"
			"\"d2LaunchResolved\":%s,\"menuPumpHooked\":%s,"
			"\"charSelectReady\":%s,\"selectedCharIndex\":%d,\"charListLoaded\":%s}",
			g_bridge.available ? "true" : "false",
			g_bridge.available ? g_bridge.getCount() : 0,
			JStr(g_providerStatus).c_str(), g_reloadSeq,
			D2Prof_Hooked(), D2Prof_Skipped(), D2Prof_Count(),
			(unsigned)(uintptr_t)D2Capture_LastUnit(), D2Capture_Count(),
			d2LaunchResolved ? "true" : "false",
			D2Action_IsPumpHookInstalled() ? "true" : "false",
			charSelReady ? "true" : "false", (int)charIdx,
			listLoaded ? "true" : "false");
		return buf;
	}

	// /dispatchers  (GET list | POST /dispatchers/mode set-all)
	if (seg[0] == "dispatchers")
	{
		if (!g_bridge.available) return ErrJson("bridge unavailable");
		const int n = g_bridge.getCount();
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
			for (int i = 0; i < n; ++i) g_bridge.setMode(i, mode);
			return std::string("{\"ok\":true,\"set\":") + std::to_string(n) + ",\"mode\":\"" + ModeName(mode) + "\"}";
		}
		return ErrJson("bad /dispatchers route");
	}

	// /dispatcher/{i}  (GET detail | POST /dispatcher/{i}/mode)
	if (seg[0] == "dispatcher" && seg.size() >= 2)
	{
		if (!g_bridge.available) return ErrJson("bridge unavailable");
		const int n = g_bridge.getCount();
		int i = atoi(seg[1].c_str());
		if (i < 0 || i >= n) return ErrJson("dispatcher index out of range");
		if (seg.size() == 2 && method == "GET")
			return std::string("{\"ok\":true,\"dispatcher\":") + DispatcherJson(i) + "}";
		if (seg.size() == 3 && seg[2] == "mode" && method == "POST")
		{
			JP jp(body); JVal v = jp.val();
			int mode = ParseMode(v.find("mode"));
			if (mode < 0) return ErrJson("bad mode (want 0|1|2 or original|reimpl|shadow)");
			std::lock_guard<std::mutex> lk(g_mcpMutex);
			g_bridge.setMode(i, mode);
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
		const int n = g_bridge.getCount();
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
		CoordFn orig = (CoordFn)g_bridge.getTrampoline(i);
		CoordFn re   = (CoordFn)g_bridge.getReimpl(i);
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
		std::string o = "{\"ok\":true,\"name\":" + JStr(g_bridge.getName(i)) +
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
					if (oa.bytes > 256) oa.bytes = 256;
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
		// (ORDINAL_RECONCILIATION.md) -- only a verified offset/address is safe:
		//   "offset": D2Common-relative (added to base 0x6fd50000), or
		//   "addr":   absolute game address.
		// Fallback: dispatcher trampoline (if hooked) else name via the
		// verified-address resolver.
		const unsigned int kD2CommonBase = 0x6fd50000u;
		const int di = DispatcherIndexForName(name.c_str());
		void* orig = nullptr;
		if (const JVal* jo = spec.find("offset"))
		{ if (jo->type == JVal::NUM) orig = (void*)(uintptr_t)(kD2CommonBase + (unsigned int)jo->num); }
		if (!orig) if (const JVal* ja = spec.find("addr"))
		{ if (ja->type == JVal::NUM) orig = (void*)(uintptr_t)(unsigned int)ja->num; }
		if (!orig)
			orig = (di >= 0 && g_bridge.getTrampoline) ? g_bridge.getTrampoline(di)
				: (g_bridge.resolveGameFn ? g_bridge.resolveGameFn(name.c_str()) : nullptr);
		void* reimpl = g_provider ? (void*)MemoryGetProcAddress(g_provider, name.c_str()) : nullptr;
		if (!reimpl && di >= 0 && g_bridge.getReimpl) reimpl = g_bridge.getReimpl(di);
		if (!orig)   return ErrJson("original not resolved (supply offset/addr, or a name in the verified-address table)");
		if (!reimpl) return ErrJson("reimpl not found (provider must export this name)");

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
		auto runOne = [&](void* fn, const JVal& vec, bool useRegs, uint64_t& retOut, std::vector<uint64_t>& bufOut)
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
			bufOut.assign(args.size(), 0);
			for (size_t k = 0; k < args.size(); ++k) if (args[k].isBuf)
			{
				uint64_t val = 0; int w = args[k].bytes < 8 ? args[k].bytes : 8;
				for (int b = 0; b < w; ++b) val |= (uint64_t)bufs[k][b] << (8 * b);
				bufOut[k] = val;
			}
		};

		int matches = 0;
		std::string results = "[";
		for (size_t vi = 0; vi < vecs->arr.size(); ++vi)
		{
			const JVal& vec = vecs->arr[vi];
			uint64_t retO = 0, retR = 0; std::vector<uint64_t> bO, bR;
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
				bO.assign(args.size(), 0); bR.assign(args.size(), 0);
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
						if (bO[k] != bR[k]) m = false;
						char b[160];
						_snprintf_s(b, sizeof(b), _TRUNCATE, "%s\"%s\":{\"o\":%llu,\"r\":%llu}",
							bufsJson.empty() ? "" : ",", c.c_str(),
							(unsigned long long)bO[k], (unsigned long long)bR[k]);
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
		ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.35f, 1.0f), "| Bridge: %d dispatchers", g_bridge.getCount());
	else
		ImGui::TextColored(ImVec4(0.90f, 0.55f, 0.25f, 1.0f), "| Bridge: none");

	// --- Global dispatch controls ---
	if (g_bridge.available)
	{
		const int nb = g_bridge.getCount();
		if (ImGui::Button("Shadow all dispatchers")) { for (int i = 0; i < nb; ++i) g_bridge.setMode(i, 2); }
		ImGui::SameLine();
		if (ImGui::Button("Original all dispatchers")) { for (int i = 0; i < nb; ++i) g_bridge.setMode(i, 0); }
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
		if (bi >= 0 && g_bridge.available) return g_bridge.getHits(bi);
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
					int mode = g_bridge.getMode(bi);
					if (ImGui::Combo("##m", &mode, kModeItems, IM_ARRAYSIZE(kModeItems)))
						g_bridge.setMode(bi, mode);
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
					const unsigned long long d = g_bridge.getDiv(bi);
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
	}
	ImGui::EndChild();

	ImGui::End();
}
