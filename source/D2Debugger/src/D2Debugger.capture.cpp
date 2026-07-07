// D2Debugger.capture.cpp -- LIVE GAME-OBJECT HANDLE CAPTURE (stateful frontier).
//
// Game objects (units/rooms) are heap-allocated at runtime -- they're in no
// static table, so the oracle can't resolve one by name. Instead we CAPTURE a
// real live pointer as the game passes it through a frequently-called function,
// and expose it so the oracle can hand it to a stateful function under test.
//
// Target: UNIT_GetMode (game D2Common+0x34870), __stdcall(UnitAny* pUnit) -- the
// game calls it constantly during play (AI/animation/render), so pUnit@[esp+4]
// is a live unit. A tiny Detours hook stashes it. The captured pointer is only
// ever READ (never written), and the oracle calls original+reimpl with it
// back-to-back under SEH -- so a stale pointer can't crash the game, only yield
// a caught exception.
#include <Windows.h>
#include <detours.h>
#include <cstdint>

namespace
{
	volatile void* g_capturedUnit = nullptr;
	volatile long  g_captureCount = 0;
	const uint32_t kD2CommonBase = 0x6fd50000u;
	const uint32_t kUnitGetModeOffset = 0x34870u; // UNIT_GetMode, __stdcall(pUnit)

	// Trampoline slot: the target address before DetourAttach, the trampoline
	// after. The capture stub jmps through it into the real function.
	void* g_tramp = nullptr;

	// __stdcall(arg0) capture stub: pUnit is at [esp+4] (ret addr at [esp]).
	// Read-only grab + count, then tail-jump to the original. Touches no C++
	// unwinding state, so a naked function is safe.
	__declspec(naked) void CaptureStub()
	{
		__asm {
			mov eax, [esp + 4]          // arg0 = pUnit (stdcall, on stack)
			mov g_capturedUnit, eax
			lock inc g_captureCount
			jmp [g_tramp]               // into the real UNIT_GetMode
		}
	}
}

// Attach the capture hook. Call once at startup after D2Common is loaded.
void D2Capture_Init()
{
	static bool done = false;
	if (done) return;
	done = true;
	g_tramp = (void*)(uintptr_t)(kD2CommonBase + kUnitGetModeOffset);
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&g_tramp, (PVOID)CaptureStub);
	DetourTransactionCommit();
}

void* D2Capture_LastUnit() { return (void*)g_capturedUnit; }
unsigned D2Capture_Count() { return (unsigned)g_captureCount; }
