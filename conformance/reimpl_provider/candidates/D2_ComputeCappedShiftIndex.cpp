#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ComputeCappedShiftIndex
// Pure arithmetic on implicit EAX input; no global state accessed.
// Algorithm: uVar1 = (EAX >> 5); if (7 < uVar1) uVar1 = 8; return uVar1;
// Cap value is 8 (0x8); shift amount is 5 (divide by 32).
extern "C" uint32_t __stdcall ComputeCappedShiftIndex(uint32_t dwInputValue)
{
    uint32_t uVar1 = dwInputValue >> 5;
    if (7u < uVar1) {
        uVar1 = 8u;
    }
    return uVar1;
}
