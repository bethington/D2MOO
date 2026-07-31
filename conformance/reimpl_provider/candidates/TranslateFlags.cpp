#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: TranslateFlags
//
// Pure flag-translation function -- reads NO global state.
// Reimpl is a literal port of the decompiled algorithm.
//
//   1   -> 0x10
//   4   -> 8
//   8   -> 4
//   0x10 -> 2
//   0x20 -> 1
//   2   -> 0x80000
//   bits 10-11 (0xC00) -> 0x100 / 0x200 / 0x300 (when non-zero)
//   bits  8-9  (0x300) -> 0x20000 if ==0, 0x10000 if ==0x200
//   bit  12   (0x1000) -> 0x40000
//
extern "C" uint32_t __cdecl TranslateFlags(uint32_t dwFlags)
{
    uint32_t dwResult = 0;

    if ((dwFlags & 0x1u) != 0) {
        dwResult = 0x10u;
    }
    if ((dwFlags & 0x4u) != 0) {
        dwResult = dwResult | 0x8u;
    }
    if ((dwFlags & 0x8u) != 0) {
        dwResult = dwResult | 0x4u;
    }
    if ((dwFlags & 0x10u) != 0) {
        dwResult = dwResult | 0x2u;
    }
    if ((dwFlags & 0x20u) != 0) {
        dwResult = dwResult | 0x1u;
    }
    if ((dwFlags & 0x2u) != 0) {
        dwResult = dwResult | 0x80000u;
    }

    {
        uint32_t dwFlags0C00 = dwFlags & 0xC00u;
        if (dwFlags0C00 != 0) {
            if (dwFlags0C00 == 0x400u) {
                dwResult = dwResult | 0x100u;
            }
            else if (dwFlags0C00 == 0x800u) {
                dwResult = dwResult | 0x200u;
            }
            else if (dwFlags0C00 == 0xC00u) {
                dwResult = dwResult | 0x300u;
            }
        }
    }

    if ((dwFlags & 0x300u) == 0) {
        dwResult = dwResult | 0x20000u;
    }
    else if ((dwFlags & 0x300u) == 0x200u) {
        dwResult = dwResult | 0x10000u;
    }

    if ((dwFlags & 0x1000u) != 0) {
        dwResult = dwResult | 0x40000u;
    }

    return dwResult;
}
