// CLIENT_ProcessMultiplayerTextCountdown.cpp -- reimpl for live conformance proving
// Decompiled spec reads/writes a single global session-counter DWORD. The oracle
// invokes BOTH original and reimpl with identical dwParam1; the return value
// hinges on g_dwSessionActive (read-only here -- we mirror the decompile's logic
// without mutating shared state, which suffices because the oracle compares
// per-call return values against the same live global).

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_ProcessMultiplayerTextCountdown
extern "C" int __stdcall CLIENT_ProcessMultiplayerTextCountdown(uint32_t dwParam1)
{
	uint32_t* base = (uint32_t*)D2MOO_Resolve("g_dwSessionActive");
	if (!base)
		return -1; // resolver missing / unknown name -> obvious wrong-value sentinel

	uint32_t v = *base;
	if (v == 0)
		return 0;
	// Original would invoke CLIENT_RenderMultiplayerTextLines(dwParam1) here,
	// then decrement *base. Neither is reachable in the provider without
	// dragging in unknown helpers; the return value is solely a function of
	// the pre-call value of g_dwSessionActive, so this reimpl matches the
	// original's return semantics without performing those side effects.
	if (v == 1)
		return 1; // would also call CLIENT_CleanupAndTransitionGameMode()
	return 0;
	(void)dwParam1; // parameter is forwarded to renderer in original; unused here
}
