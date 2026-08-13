// D2Debugger.vinput.moveto.cpp -- move the game's cursor, and CONFIRM it landed.
//
// WHY THIS ONE FUNCTION DID NOT MOVE WITH THE REST OF vinput.
//
// The rest of the virtual-input layer now lives in d2-fleet/patch/vinput.cpp,
// compiled into both binaries. That file goes through Windows API seams
// exclusively, which is what lets ONE D2FleetHook.dll work on all 28 catalogued
// versions from 1.00 to PD2.
//
// The confirmation below does not: it reads D2Client's mouse-position globals
// at RVAs taken from a 1.13c build. On any other version those addresses are
// something else entirely, and reading them is at best a wrong answer and at
// worst a fault in a game the fleet is trying to keep alive. So the shared file
// keeps the SEND (D2VInput_PostMouseMove, which is version-agnostic) and this
// build-specific verification stays here, in the binary that is already pinned
// to the build its ordinals came from.
//
// MEASURED 2026-07-31, and it overturned the design vinput started with.
// Instrumenting the hooks in-game gave:
//
//     GetCursorPos       0 calls        <- never polled
//     GetAsyncKeyState   1,273,833      <- hammered every frame
//     GetKeyState        0 calls
//
// So the game does NOT read its mouse POSITION from GetCursorPos. ddraw.dll
// imports the symbol, but importing is not calling -- cnc-ddraw runs with
// handlemouse=true and takes position from the MESSAGE QUEUE. Lying about the
// cursor could never have moved it, which is exactly what the first attempt
// saw: the game's mouse view sat frozen at 320,240.
//
// BUTTONS and KEYS do come through GetAsyncKeyState, so that half of the
// original design is correct and stays.
//
// The message path then turned out to be 1:1 with no scaling: posting
// WM_MOUSEMOVE at client (100,80) put the game's mouse at exactly (100,80),
// and (900,500) at (900,500). So no feedback loop, no scale estimation and no
// DPI math is needed here -- the game's coordinate space IS the window's
// client space. The loop below exists only to CONFIRM the move landed, which
// costs one readback and turns a silent no-op into a reported failure.
//
// In-process PostMessage is also what makes this usable at all: the game runs
// elevated, so the identical call from an operator shell is dropped by UIPI
// with no error.
#include <windows.h>
#include <cstdint>

// The send half, and the virtual cursor it keeps in step -- both now shared
// (d2-fleet/patch/vinput.h).
extern "C" int  D2VInput_PostMouseMove(int clientX, int clientY);
extern "C" void D2VInput_SetScreenPos(int x, int y);

extern "C" int D2VInput_MoveToGameXY(int gameX, int gameY, int* outX, int* outY)
{
	uintptr_t d2client = (uintptr_t)GetModuleHandleA("D2Client.dll");
	if (!d2client)
		return -2;
	const uintptr_t kD2ClientBase = 0x6fab0000u;
	volatile int* pMX = (volatile int*)(d2client + (0x6fbcb828u - kD2ClientBase));
	volatile int* pMY = (volatile int*)(d2client + (0x6fbcb824u - kD2ClientBase));

	if (!D2VInput_PostMouseMove(gameX, gameY))
		return -2;                      // game window not found

	// Confirm the game consumed it. It polls its queue once per frame, so a few
	// short waits is plenty; reporting a miss beats returning a false success.
	for (int i = 0; i < 10; ++i)
	{
		Sleep(20);
		const int gx = *pMX, gy = *pMY;
		if (gx == gameX && gy == gameY)
		{
			if (outX) *outX = gx;
			if (outY) *outY = gy;
			// Keep the virtual view in step. Through the layer's own setter now
			// that the state lives in another translation unit -- the file-local
			// atomics are not this file's to touch.
			D2VInput_SetScreenPos(gameX, gameY);
			return 1;
		}
	}
	if (outX) *outX = *pMX;
	if (outY) *outY = *pMY;
	return -3;                          // posted, but the game never took it
}
