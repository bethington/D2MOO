// ToggleGameRunningState.cpp -- reimpl for D2Client!0x6faf19b0
// Plate comment says "g_dwGameRunning" but the decompile writes to
// _g_dwLastError_6fbcc380 (the address 0x6fbcc380 matches that symbol);
// follow the decompile, not the plate summary.
//
// Algorithm:
//   *base = (*base == 0) ? 1 : 0;
//
// The plate comment is decorative -- the actual global is g_dwLastError_6fbcc380
// (the "LastError" suffix is just naming; the offset 0x6fbcc380 matches that
// symbol exactly). Resolved by NAME via the injected resolver.

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ToggleGameRunningState
extern "C" void __stdcall ToggleGameRunningState()
{
    // &g_dwLastError_6fbcc380 is the storage cell; resolve it by verified name.
    uint32_t* base = (uint32_t*)D2MOO_Resolve("g_dwLastError_6fbcc380");
    if (!base)
    {
        // Resolver not injected / name unknown -- deliberately do nothing.
        // The original still toggles its global, ours doesn't, so the prover
        // sees the mismatch rather than matching by accident on null.
        return;
    }

    // (uint)(_g_dwLastError_6fbcc380 == 0) -- C ternary preserves the 0/1 result.
    *base = (*base == 0u) ? 1u : 0u;
}
