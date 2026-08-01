// D2Debugger.crash.cpp -- report a D2 fault the INSTANT it happens.
//
// Before this, a crash was only ever inferred from its aftermath: the oracle
// stops answering, or the game sits alive-but-not-rendering behind a modal
// "Diablo II Exception" box, and the monitor eventually calls it wedged. By
// then the faulting address, the register state and -- crucially -- WHICH
// candidate was being proven are all gone. That is not a small loss: a crash
// during proving currently surfaces as a timeout and the function collects a
// wrong TERMINAL verdict, which is the same mechanism that retired 104
// D2Client functions whose reimplementation never executed once.
//
// THREE OBSERVATION POINTS, because no single one is sufficient:
//
//   1. A vectored handler sees the fault FIRST-CHANCE, before Fog.dll's filter
//      and before any dialog. It cannot decide the crash is fatal, though --
//      first-chance exceptions are routine (C++ throws are 0xE06D7363, and
//      guard-page hits are normal), so acting on one would cry wolf constantly.
//      It records; it does not report.
//   2. The unhandled-exception filter is what makes it terminal. But D2's own
//      Fog.dll installs a filter too and LAST WRITER WINS, so simply calling
//      SetUnhandledExceptionFilter would get us silently displaced. We detour
//      the setter itself and chain, which is the same technique used for every
//      other hook in this DLL.
//   3. MessageBoxA/W is the backstop that catches the dialog itself -- the
//      shape actually observed on screen, and the one that leaves the process
//      alive and blocked forever with no fault ever reaching a filter.
//
// DISK FIRST. The process may not survive long enough to be asked anything, so
// the record is written with raw CreateFile/WriteFile before any reporting
// state is touched. Nothing on this path allocates: a crash reporter that
// faults is strictly worse than no crash reporter, so every buffer is static
// and the whole body runs under SEH.

#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <cstring>
#include <atomic>
#include "detours.h"

namespace
{
	// Fatal codes only. Everything else is either routine control flow or
	// somebody else's business.
	bool IsFatalCode(DWORD c)
	{
		switch (c)
		{
		case EXCEPTION_ACCESS_VIOLATION:
		case EXCEPTION_ILLEGAL_INSTRUCTION:
		case EXCEPTION_PRIV_INSTRUCTION:
		case EXCEPTION_STACK_OVERFLOW:
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		case EXCEPTION_IN_PAGE_ERROR:
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			return true;
		default:
			return false;
		}
	}

	const char* CodeName(DWORD c)
	{
		switch (c)
		{
		case EXCEPTION_ACCESS_VIOLATION:      return "ACCESS_VIOLATION";
		case EXCEPTION_ILLEGAL_INSTRUCTION:   return "ILLEGAL_INSTRUCTION";
		case EXCEPTION_PRIV_INSTRUCTION:      return "PRIV_INSTRUCTION";
		case EXCEPTION_STACK_OVERFLOW:        return "STACK_OVERFLOW";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:    return "INT_DIVIDE_BY_ZERO";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "ARRAY_BOUNDS_EXCEEDED";
		case EXCEPTION_IN_PAGE_ERROR:         return "IN_PAGE_ERROR";
		case EXCEPTION_DATATYPE_MISALIGNMENT: return "DATATYPE_MISALIGNMENT";
		default:                              return "OTHER";
		}
	}

	// What the oracle is currently proving, set by the /call path. THE reason
	// this file earns its keep: a fault during proving is otherwise attributed
	// to nothing, the candidate times out, and it collects a terminal verdict
	// its reimplementation never earned. Fixed buffer, no allocation -- this is
	// read from a crashing thread.
	char g_proveCtx[256] = { 0 };

	// ---- state (all preallocated) -------------------------------------------
	constexpr int kJsonMax = 8192;
	char g_lastJson[kJsonMax] = { 0 };
	std::atomic<bool> g_haveCrash{ false };
	std::atomic<unsigned long> g_firstChanceFatal{ 0 };
	CRITICAL_SECTION g_cs;
	bool g_csReady = false;
	char g_dir[MAX_PATH] = { 0 };

	// Resolve an address to "module+rva". A raw absolute is nearly useless here:
	// D2Client does not load at its preferred base (the process maps it at
	// 0x03600000 while Ghidra shows 0x6fab0000), so an absolute cannot be
	// matched against anything static without knowing the live base.
	void DescribeAddress(void* addr, char* modOut, size_t modCap, DWORD* rvaOut)
	{
		modOut[0] = 0;
		*rvaOut = 0;
		HMODULE h = nullptr;
		if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		                       (LPCSTR)addr, &h) && h)
		{
			char full[MAX_PATH] = { 0 };
			if (GetModuleFileNameA(h, full, MAX_PATH))
			{
				const char* base = strrchr(full, '\\');
				lstrcpynA(modOut, base ? base + 1 : full, (int)modCap);
			}
			*rvaOut = (DWORD)((ULONG_PTR)addr - (ULONG_PTR)h);
		}
		else
		{
			lstrcpynA(modOut, "<unmapped>", (int)modCap);
		}
	}

	void EnsureDir()
	{
		if (g_dir[0])
			return;
		// Next to the other behavioural artefacts, so a crash record lands where
		// someone already looks.
		lstrcpynA(g_dir, "C:\\Users\\benam\\source\\cpp\\D2MOO\\conformance\\behavioral\\crashes", MAX_PATH);
		CreateDirectoryA(g_dir, nullptr);
	}

	// Raw Win32 write -- no CRT buffering, no allocation, survives a process
	// that is about to die.
	void WriteRecord(const char* json, int len)
	{
		EnsureDir();
		SYSTEMTIME st{};
		GetLocalTime(&st);
		char path[MAX_PATH] = { 0 };
		wsprintfA(path, "%s\\crash_%04d%02d%02d_%02d%02d%02d_%03d.json",
		          g_dir, st.wYear, st.wMonth, st.wDay,
		          st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
		                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (h != INVALID_HANDLE_VALUE)
		{
			DWORD wrote = 0;
			WriteFile(h, json, (DWORD)len, &wrote, nullptr);
			FlushFileBuffers(h);
			CloseHandle(h);
		}
	}

	// Build the record. `ctx` may be null (the MessageBox path has no context).
	void Record(const char* origin, DWORD code, void* addr, CONTEXT* ctx, const char* note)
	{
		char mod[64] = { 0 };
		DWORD rva = 0;
		if (addr)
			DescribeAddress(addr, mod, sizeof(mod), &rva);

		const char* prove = g_proveCtx[0] ? g_proveCtx : nullptr;

		SYSTEMTIME st{};
		GetLocalTime(&st);

		int n = wsprintfA(g_lastJson,
			"{\"origin\":\"%s\",\"code\":\"0x%08X\",\"codeName\":\"%s\","
			"\"address\":\"0x%08X\",\"module\":\"%s\",\"rva\":\"0x%08X\","
			"\"pid\":%lu,\"tid\":%lu,"
			"\"at\":\"%04d-%02d-%02dT%02d:%02d:%02d\","
			"\"firstChanceFatalSeen\":%lu",
			origin, code, CodeName(code),
			(unsigned)(ULONG_PTR)addr, mod[0] ? mod : "?", rva,
			GetCurrentProcessId(), GetCurrentThreadId(),
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
			g_firstChanceFatal.load(std::memory_order_relaxed));

#if defined(_M_IX86)
		if (ctx && n < kJsonMax - 512)
		{
			n += wsprintfA(g_lastJson + n,
				",\"regs\":{\"eip\":\"0x%08X\",\"esp\":\"0x%08X\",\"ebp\":\"0x%08X\","
				"\"eax\":\"0x%08X\",\"ebx\":\"0x%08X\",\"ecx\":\"0x%08X\","
				"\"edx\":\"0x%08X\",\"esi\":\"0x%08X\",\"edi\":\"0x%08X\"}",
				ctx->Eip, ctx->Esp, ctx->Ebp, ctx->Eax, ctx->Ebx,
				ctx->Ecx, ctx->Edx, ctx->Esi, ctx->Edi);

			// Bounded EBP-chain walk. D2 is x86 with frame pointers in the
			// overwhelming majority of frames; each read is probed first so a
			// smashed stack degrades to a short trace instead of a second fault.
			n += wsprintfA(g_lastJson + n, ",\"stack\":[");
			ULONG_PTR ebp = ctx->Ebp;
			bool first = true;
			for (int depth = 0; depth < 16 && n < kJsonMax - 128; ++depth)
			{
				if (IsBadReadPtr((void*)ebp, sizeof(ULONG_PTR) * 2))
					break;
				const ULONG_PTR ret = ((ULONG_PTR*)ebp)[1];
				const ULONG_PTR next = ((ULONG_PTR*)ebp)[0];
				if (!ret || next <= ebp)
					break;
				char m2[64] = { 0 };
				DWORD r2 = 0;
				DescribeAddress((void*)ret, m2, sizeof(m2), &r2);
				n += wsprintfA(g_lastJson + n, "%s{\"module\":\"%s\",\"rva\":\"0x%08X\"}",
				               first ? "" : ",", m2[0] ? m2 : "?", r2);
				first = false;
				ebp = next;
			}
			n += wsprintfA(g_lastJson + n, "]");

			// HEURISTIC SCAN -- what a real debugger falls back to when the
			// frame pointer is gone.
			//
			// The EBP chain above produced NOTHING for the crash this was built
			// to explain, because ebp was 0x00000001: once the frame pointer is
			// smashed there is no chain left to walk. But the return addresses
			// are still lying on the stack. Scanning upward from ESP and keeping
			// every value that resolves inside a loaded module recovers a
			// probable call chain -- not provably ordered like a real backtrace,
			// which is why it is reported separately from "stack" rather than
			// dressed up as one.
			//
			// Deliberately skips addresses inside THIS module: the top of the
			// stack is full of our own reporting frames, which are noise.
			n += wsprintfA(g_lastJson + n, ",\"stackScan\":[");
			HMODULE self = nullptr;
			GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			                   GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			                   (LPCSTR)&Record, &self);
			const ULONG_PTR* sp = (const ULONG_PTR*)ctx->Esp;
			bool firstScan = true;
			int kept = 0;
			for (int i = 0; i < 256 && kept < 24 && n < kJsonMax - 160; ++i)
			{
				if (IsBadReadPtr(sp + i, sizeof(ULONG_PTR)))
					break;
				const ULONG_PTR v = sp[i];
				if (v < 0x10000)
					continue;                       // too low to be code
				HMODULE h = nullptr;
				if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
				                        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
				                        (LPCSTR)v, &h) || !h || h == self)
					continue;
				char mn[64] = { 0 };
				DWORD rv = 0;
				DescribeAddress((void*)v, mn, sizeof(mn), &rv);
				n += wsprintfA(g_lastJson + n,
				               "%s{\"at\":\"esp+0x%X\",\"module\":\"%s\",\"rva\":\"0x%08X\"}",
				               firstScan ? "" : ",", i * 4, mn[0] ? mn : "?", rv);
				firstScan = false;
				++kept;
			}
			n += wsprintfA(g_lastJson + n, "]");

			// Raw words around the fault, so an address that resolves to no
			// module at all (which is this crash -- eip was ON the stack) can
			// still be reasoned about from the bytes themselves.
			n += wsprintfA(g_lastJson + n, ",\"stackWords\":[");
			for (int i = 0; i < 16 && n < kJsonMax - 64; ++i)
			{
				if (IsBadReadPtr(sp + i, sizeof(ULONG_PTR)))
					break;
				n += wsprintfA(g_lastJson + n, "%s\"0x%08X\"",
				               i ? "," : "", (unsigned)sp[i]);
			}
			n += wsprintfA(g_lastJson + n, "]");
		}
#endif
		if (prove && n < kJsonMax - 300)
			n += wsprintfA(g_lastJson + n, ",\"proving\":\"%.200s\"", prove);
		if (note && n < kJsonMax - 300)
		{
			// ESCAPE IT. The MessageBox text is multi-line -- "UNHANDLED
			// EXCEPTION:" then the code on the next line -- and a raw newline
			// inside a JSON string makes the record unparseable. Measured: the
			// first real crash this ever caught wrote a file json.load could not
			// read, which is a poor way to find out about a crash.
			n += wsprintfA(g_lastJson + n, ",\"note\":\"");
			for (int i = 0; note[i] && i < 200 && n < kJsonMax - 8; ++i)
			{
				const unsigned char c = (unsigned char)note[i];
				if (c == '\n')      { g_lastJson[n++] = '\\'; g_lastJson[n++] = 'n'; }
				else if (c == '\r') { g_lastJson[n++] = '\\'; g_lastJson[n++] = 'r'; }
				else if (c == '\t') { g_lastJson[n++] = '\\'; g_lastJson[n++] = 't'; }
				else if (c == '"' || c == '\\')
				                    { g_lastJson[n++] = '\\'; g_lastJson[n++] = (char)c; }
				else if (c >= 0x20) { g_lastJson[n++] = (char)c; }
				// other control characters are dropped
			}
			g_lastJson[n++] = '"';
			g_lastJson[n] = 0;
		}
		n += wsprintfA(g_lastJson + n, "}");

		WriteRecord(g_lastJson, n);
		g_haveCrash.store(true, std::memory_order_release);
	}

	// ---- 1. first-chance observer -------------------------------------------
	LONG CALLBACK VectoredHandler(EXCEPTION_POINTERS* ep)
	{
		if (ep && ep->ExceptionRecord && IsFatalCode(ep->ExceptionRecord->ExceptionCode))
			g_firstChanceFatal.fetch_add(1, std::memory_order_relaxed);
		return EXCEPTION_CONTINUE_SEARCH;     // observe only, never handle
	}

	// ---- 2. terminal filter, chained ----------------------------------------
	LPTOP_LEVEL_EXCEPTION_FILTER g_prevFilter = nullptr;

	// Write a real MINIDUMP next to the JSON record.
	//
	// This is the thing that makes "a stack trace every time" actually true. The
	// JSON carries registers and a heuristic scan; a minidump carries every
	// thread's stack and can be opened by cdb/WinDbg for a SYMBOLISED backtrace,
	// which is what was wanted and what the EBP walk could never deliver once
	// the frame pointer was smashed.
	//
	// Done here rather than through Windows Error Reporting because WER only
	// fires for an UNHANDLED exception -- and D2 installs its own filter that
	// shows the "Diablo II Exception" box, which HANDLES the fault. WER would
	// therefore write nothing, silently, which is the worst kind of monitoring.
	//
	// dbghelp is loaded lazily and the call is SEH-guarded: a crash reporter
	// that faults is worse than none, and this runs in an already-broken process.
	void WriteMiniDump(EXCEPTION_POINTERS* ep)
	{
		HMODULE dbghelp = LoadLibraryA("dbghelp.dll");
		if (!dbghelp)
			return;
		using WriteFn = BOOL(WINAPI*)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
		                              PMINIDUMP_EXCEPTION_INFORMATION,
		                              PMINIDUMP_USER_STREAM_INFORMATION,
		                              PMINIDUMP_CALLBACK_INFORMATION);
		auto writeDump = (WriteFn)GetProcAddress(dbghelp, "MiniDumpWriteDump");
		if (!writeDump)
			return;

		EnsureDir();
		SYSTEMTIME st{};
		GetLocalTime(&st);
		char path[MAX_PATH] = { 0 };
		wsprintfA(path, "%s\crash_%04d%02d%02d_%02d%02d%02d_%03d.dmp",
		          g_dir, st.wYear, st.wMonth, st.wDay,
		          st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

		HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
		                       FILE_ATTRIBUTE_NORMAL, nullptr);
		if (f == INVALID_HANDLE_VALUE)
			return;

		MINIDUMP_EXCEPTION_INFORMATION mei{};
		mei.ThreadId = GetCurrentThreadId();
		mei.ExceptionPointers = ep;
		mei.ClientPointers = FALSE;

		// Thread stacks + module list + handles. Deliberately NOT full memory:
		// stacks are what a backtrace needs, and full dumps of this game would
		// be hundreds of MB each.
		const MINIDUMP_TYPE type = (MINIDUMP_TYPE)(
			MiniDumpWithIndirectlyReferencedMemory |
			MiniDumpWithProcessThreadData |
			MiniDumpWithHandleData);

		writeDump(GetCurrentProcess(), GetCurrentProcessId(), f, type,
		          ep ? &mei : nullptr, nullptr, nullptr);
		CloseHandle(f);
	}

	LONG WINAPI OurFilter(EXCEPTION_POINTERS* ep)
	{
		__try { WriteMiniDump(ep); }
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		__try
		{
			if (ep && ep->ExceptionRecord)
				Record("unhandled_filter",
				       ep->ExceptionRecord->ExceptionCode,
				       ep->ExceptionRecord->ExceptionAddress,
				       ep->ContextRecord, nullptr);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) { /* never fault inside the reporter */ }

		// Hand back to whoever was there before us. Suppressing D2's own filter
		// would change crash BEHAVIOUR, and this is an observer.
		if (g_prevFilter)
			return g_prevFilter(ep);
		return EXCEPTION_CONTINUE_SEARCH;
	}

	using SetFilterFn = LPTOP_LEVEL_EXCEPTION_FILTER(WINAPI*)(LPTOP_LEVEL_EXCEPTION_FILTER);
	SetFilterFn g_realSetFilter = nullptr;

	// Fog.dll installs its filter during init, AFTER us. Without this detour it
	// would replace ours outright and this file would silently never fire.
	LPTOP_LEVEL_EXCEPTION_FILTER WINAPI H_SetUnhandledExceptionFilter(
		LPTOP_LEVEL_EXCEPTION_FILTER f)
	{
		LPTOP_LEVEL_EXCEPTION_FILTER prev = g_prevFilter;
		g_prevFilter = f;                 // chain to the game's filter
		g_realSetFilter(OurFilter);       // but stay on top ourselves
		return prev;
	}

	// ---- 3. dialog backstop --------------------------------------------------
	using MsgBoxAFn = int (WINAPI*)(HWND, LPCSTR, LPCSTR, UINT);
	using MsgBoxWFn = int (WINAPI*)(HWND, LPCWSTR, LPCWSTR, UINT);
	MsgBoxAFn g_realMsgBoxA = nullptr;
	MsgBoxWFn g_realMsgBoxW = nullptr;

	bool LooksLikeD2Fault(const char* title, const char* text)
	{
		// Match on either word: the dialog actually observed is titled "Diablo II
		// Exception", and a checker that only looked for "Error" is exactly why
		// it was never recognised.
		auto has = [](const char* h, const char* n) {
			if (!h || !n) return false;
			for (const char* p = h; *p; ++p)
			{
				int i = 0;
				while (n[i] && p[i] &&
				       (p[i] | 32) == (n[i] | 32)) ++i;
				if (!n[i]) return true;
			}
			return false;
		};
		return (has(title, "diablo") && (has(title, "error") || has(title, "exception")))
		    || has(text, "UNHANDLED EXCEPTION");
	}

	int WINAPI H_MessageBoxA(HWND h, LPCSTR text, LPCSTR cap, UINT t)
	{
		__try
		{
			if (LooksLikeD2Fault(cap, text))
				Record("message_box", 0, nullptr, nullptr, text ? text : "");
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		return g_realMsgBoxA(h, text, cap, t);
	}

	int WINAPI H_MessageBoxW(HWND h, LPCWSTR text, LPCWSTR cap, UINT t)
	{
		__try
		{
			char a[256] = { 0 }, c[128] = { 0 };
			if (text) WideCharToMultiByte(CP_UTF8, 0, text, -1, a, sizeof(a) - 1, nullptr, nullptr);
			if (cap)  WideCharToMultiByte(CP_UTF8, 0, cap,  -1, c, sizeof(c) - 1, nullptr, nullptr);
			if (LooksLikeD2Fault(c, a))
				Record("message_box", 0, nullptr, nullptr, a);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		return g_realMsgBoxW(h, text, cap, t);
	}
}

// ---------------------------------------------------------------- public ----

extern "C" void D2Crash_Install()
{
	static bool done = false;
	if (done)
		return;
	done = true;

	// OPT-IN. Set D2DBG_CRASH=1 to enable.
	//
	// PRECAUTIONARY, NOT PROVEN. The game began dying at startup -- about ten
	// seconds in, before presenting a single frame, with an identical fault
	// every launch (eip on the stack: eip=0x0019F968, esp=0x0019F7D0, ebp=1,
	// across six processes). A plain uninjected Game.exe was fine throughout, so
	// the game itself was never the problem; something we inject is.
	//
	// The bisect that appeared to implicate THIS file was invalid, and it is
	// worth recording why so the mistake is not repeated. Two detectors were
	// used and both were wrong:
	//   * "a crash record appeared" -- records are only written when this
	//     observer is installed, and the observer was one of the variables;
	//   * "the process is still alive" -- D2's fault dialog is MODAL, so a
	//     crashed game stays alive precisely BECAUSE it has crashed.
	// The only sound signal is whether StretchBlt keeps climbing, i.e. whether
	// the game is still presenting frames. Measured that way, every
	// configuration tested was still crashing.
	//
	// So the cause is unknown. This subsystem is the newest and by far the most
	// invasive thing here -- it interposes on SetUnhandledExceptionFilter, which
	// changes which exceptions the game treats as fatal -- so it defaults off
	// until it is cleared. A crash reporter that might cause crashes is worse
	// than no crash reporter.
	if (GetEnvironmentVariableA("D2DBG_CRASH", nullptr, 0) == 0)
		return;

	InitializeCriticalSection(&g_cs);
	g_csReady = true;

	AddVectoredExceptionHandler(1 /*first*/, VectoredHandler);

	g_realSetFilter = (SetFilterFn)DetourFindFunction("kernel32.dll", "SetUnhandledExceptionFilter");
	g_realMsgBoxA   = (MsgBoxAFn)DetourFindFunction("user32.dll", "MessageBoxA");
	g_realMsgBoxW   = (MsgBoxWFn)DetourFindFunction("user32.dll", "MessageBoxW");

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	if (g_realSetFilter) DetourAttach(&(PVOID&)g_realSetFilter, H_SetUnhandledExceptionFilter);
	if (g_realMsgBoxA)   DetourAttach(&(PVOID&)g_realMsgBoxA,   H_MessageBoxA);
	if (g_realMsgBoxW)   DetourAttach(&(PVOID&)g_realMsgBoxW,   H_MessageBoxW);
	DetourTransactionCommit();

	// Install ours now; the detour above keeps us on top of anyone who follows.
	if (g_realSetFilter)
		g_prevFilter = g_realSetFilter(OurFilter);
}

// Called by the oracle around a live call so a fault can be blamed on the
// candidate that caused it rather than on whatever timed out afterwards.
extern "C" void D2Crash_SetProveContext(const char* what)
{
	if (!what || !*what)
		g_proveCtx[0] = 0;
	else
		lstrcpynA(g_proveCtx, what, (int)sizeof(g_proveCtx));
}

extern "C" const char* D2Crash_ActiveProveContext()
{
	return g_proveCtx[0] ? g_proveCtx : nullptr;
}

// Latest crash record as JSON, or "null". Served by GET /crash.
extern "C" const char* D2Crash_LastJson()
{
	return g_haveCrash.load(std::memory_order_acquire) ? g_lastJson : "null";
}

extern "C" int D2Crash_Have() { return g_haveCrash.load(std::memory_order_acquire) ? 1 : 0; }

extern "C" unsigned long D2Crash_FirstChanceFatalCount()
{
	return g_firstChanceFatal.load(std::memory_order_relaxed);
}

// Clear after a consumer has taken it, so the same crash is not re-reported on
// every poll. The on-disk record is the durable copy and is never removed.
extern "C" void D2Crash_Clear()
{
	g_haveCrash.store(false, std::memory_order_release);
}
