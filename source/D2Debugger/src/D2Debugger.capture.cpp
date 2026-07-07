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

namespace
{
	volatile void* g_capStack = nullptr;  // [esp+4] at entry (stdcall/cdecl arg0)
	volatile void* g_capEcx   = nullptr;  // ecx at entry (fastcall/thiscall arg0)
	volatile long  g_captureCount = 0;
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
			jmp  [g_tramp]
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
