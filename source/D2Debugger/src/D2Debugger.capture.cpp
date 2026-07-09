// D2Debugger.capture.cpp -- LIVE GAME-OBJECT HANDLE CAPTURE (stateful frontier).
//
// Game objects (units/rooms) are heap-allocated at runtime -- in no static
// table -- so the oracle can't resolve one by name. Instead we CAPTURE a real
// live pointer as the game passes it through a frequently-called function, and
// hand it to the oracle for a stateful function under test.
//
// RUNTIME-RETARGETABLE: the capture target is chosen live via POST /capture
// {offset, ecx}, so we can hunt for a HOT non-inlined function (getters like
// UNIT_GetMode are inlined by the game and never fire) WITHOUT rebuilding. The
// stub grabs BOTH the first stack slot ([esp+4], stdcall/cdecl arg0) AND ECX
// (fastcall/thiscall arg0); the accessor returns whichever matches the target's
// convention. The captured pointer is only READ, and the oracle calls under
// SEH, so a stale/garbage pointer yields a caught exception, never a crash.
#include <Windows.h>
#include <detours.h>
#include <cstdint>

// Game-thread call queue pump (D2Debugger.gtqueue.cpp) -- drained here because
// this hook runs on the game thread every frame in-world. Cheap when idle.
extern "C" void D2Gt_Pump();
// Remembers one representative captured object PER dwType (branch-coverage
// diversity) -- see D2Capture_Note below. extern "C" so the naked stub can call it.
extern "C" void D2Capture_Note(void* p);

namespace
{
	volatile void* g_capStack = nullptr;  // [esp+4] at entry (stdcall/cdecl arg0)
	volatile void* g_capEcx   = nullptr;  // ecx at entry (fastcall/thiscall arg0)
	volatile long  g_captureCount = 0;
	// Last captured object of each dwType (index = dwType at +0). Lets the oracle
	// run a proof across DISTINCT unit types -> exercise all type-dispatched
	// branches even when the hook keeps seeing the same unit (the standing-still
	// case that made branch coverage collapse to one type).
	void* g_capByType[8] = {};
	const uint32_t kD2CommonBase = 0x6fd50000u;

	void* g_tramp = nullptr;        // target addr before attach; trampoline after
	bool  g_attached = false;
	bool  g_useEcx = false;         // which reg holds the object for this target
	uint32_t g_targetOffset = 0;

	// Grabs both candidate arg0 registers, counts, tail-jumps to the original.
	__declspec(naked) void CaptureStub()
	{
		__asm {
			mov  eax, [esp + 4]
			mov  g_capStack, eax
			mov  g_capEcx, ecx
			lock inc g_captureCount
			push eax                // remember this object by its dwType (branch diversity)
			call D2Capture_Note     // extern "C" cdecl void(void*); caller cleans up
			add  esp, 4
			call D2Gt_Pump          // drain the game-thread call queue (cheap if idle)
			jmp  [g_tramp]          // eax/ecx/edx now scratch; target reads pUnit from [esp+4]
		}
	}
}

// (Re)point the capture hook at D2Common+offset. useEcx selects which reg the
// accessor returns (fastcall/thiscall -> ecx; stdcall/cdecl -> stack). Detaches
// any previous target and resets counters. Returns true on a clean commit.
bool D2Capture_Attach(uint32_t offset, bool useEcx)
{
	LONG err = 0;
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	if (g_attached)
		DetourDetach(&g_tramp, (PVOID)CaptureStub);
	g_tramp = (void*)(uintptr_t)(kD2CommonBase + offset);
	DetourAttach(&g_tramp, (PVOID)CaptureStub);
	err = DetourTransactionCommit();
	g_attached = (err == NO_ERROR);
	g_useEcx = useEcx;
	g_targetOffset = offset;
	g_capStack = g_capEcx = nullptr;
	g_captureCount = 0;
	return g_attached;
}

void D2Capture_Init()
{
	// Default-attach to a proven-HOT target: UNIT_UpdateUnitCollisionOrLight
	// (D2Common+0x35800, __stdcall(UnitAny*)) -- the game calls it constantly
	// in-world for units near the player, so a live unit is captured with no
	// manual /capture. Retarget any time via POST /capture. (UNIT_GetMode was
	// proven live 2026-07-07 against a unit captured here.)
	D2Capture_Attach(0x35800u, /*useEcx=*/false);
}

void*    D2Capture_LastUnit() { return (void*)(g_useEcx ? g_capEcx : g_capStack); }
unsigned D2Capture_Count()    { return (unsigned)g_captureCount; }
unsigned D2Capture_Offset()   { return g_targetOffset; }
bool     D2Capture_Attached()  { return g_attached; }

// Note a captured object by its dwType (+0, SEH-guarded so a stale/garbage arg is
// ignored). Called from CaptureStub on every hook fire; keeps one representative
// per type so a proof can run across DISTINCT types for full branch coverage.
extern "C" void D2Capture_Note(void* p)
{
	if (!p) return;
	unsigned t;
	__try { t = *(volatile unsigned*)p; }          // dwType at +0
	__except (EXCEPTION_EXECUTE_HANDLER) { return; }
	if (t < 8) g_capByType[t] = p;
}

// Fill `out` with the distinct captured objects (one per dwType seen); returns the
// count. The oracle runs vector i against out[i % count] so ONE proof exercises
// every currently-available unit type -> maximal branch coverage with no gameplay.
extern "C" int D2Capture_FillDistinct(void** out, int max)
{
	int n = 0;
	for (int t = 0; t < 8 && n < max; ++t)
		if (g_capByType[t]) out[n++] = g_capByType[t];
	return n;
}
