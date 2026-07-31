#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: IsWindowedModeEnabled
extern "C" int __stdcall IsWindowedModeEnabled()
{
	// Read-only: observe the cached state set by the original's lazy init
	// (g_bWindowModeLoaded/SRegLoadValue path runs only in the original; the
	// reimpl just reads the same live global the original populates).
	// NEEDS GLOBAL: g_dwWindowModeState
	uint32_t* statePtr = (uint32_t*)D2MOO_Resolve("g_dwWindowModeState");
	if (!statePtr)
		return 0; // resolver missing / name unknown -> obvious mismatch
	return (*statePtr) != 0;
}
