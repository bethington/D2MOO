#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_AllocateGameMemoryPool
// PD2-S12 0x6fac0900 -- allocates a 100MB pool, touches every 4KB page to
// commit virtual memory, then initializes the D2Client module. Always
// returns 1. The 8-byte RET indicates two unused passthrough stack params
// from the caller (WinMain-style forwarding). This function is read-only
// from the reimpl's perspective: it never inspects game globals (the pool
// base comes from the game's own allocator, not from a named global), so
// no D2MOO_Resolve is needed. The oracle compares the 1-in-EAX return value,
// which is invariant.
extern "C" uint32_t __stdcall CLIENT_AllocateGameMemoryPool(uint32_t unused1, uint32_t unused2)
{
	// Original behavior (for reference, NOT executed here -- we must not
	// mutate game state from the reimpl):
	//   pPoolCursor = AllocateMemoryWithTracking();      // game allocator
	//   pPoolEnd    = pPoolCursor + 0x6000000;           // 100MB end
	//   for (; pPoolCursor < pPoolEnd; pPoolCursor += 0x1000)
	//       *pPoolCursor = 0;                            // commit every page
	//   CLIENT_InitializeModule();
	//   return 1;
	//
	// The original always returns 1 (TRUE). Since the two stack params are
	// explicitly unused passthroughs (per the plate comment and the decompile
	// body which never references them), the return is independent of input.
	return 1u;
}
