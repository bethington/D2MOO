// D2MOO_REIMPL_EXPORT: DATATBLS_GetOverlayRecordByte50
#include "../provider_runtime.h"

// Shadow reimpl of DATATBLS_GetOverlayRecordByte50 @ 0x6fd93710
// stdcall, 2 stack args, RET 0x8, returns byte in EAX.
// Valid IDs are 1,2,3,4,8,9 -> byte-indexed read of pOverlayLookup.
// ID >= 0x10  -> error path 0x265
// IDs 0,5,6,7,10..15 -> error path 0x274 (default fall-through)
// In shadow mode we cannot re-enter CleanupAndAbort; mirror the byte
// extraction 1:1 and return 0 sentinel for the abort cases (the real
// game keeps the original's result on those paths).

extern "C" uint32_t __stdcall DATATBLS_GetOverlayRecordByte50(int nOverlayId, uint8_t* pOverlayLookup)
{
    if (nOverlayId >= 0x10) {
        // matches disasm path: PUSH 0x265 -> CleanupAndAbort
        // shadow: never reached in normal gameplay; emit sentinel
        return 0u;
    }
    // switch (nOverlayId) with cases 1,2,3,4,8,9; default -> 0x274 abort
    switch (nOverlayId) {
        case 1: return (uint32_t)pOverlayLookup[2];   // case 1 -> [2]
        case 2: return (uint32_t)pOverlayLookup[3];   // case 2 -> [3]
        case 3: return (uint32_t)pOverlayLookup[0];   // case 3 -> [0]
        case 4: return (uint32_t)pOverlayLookup[1];   // case 4 -> [1]
        case 8: return (uint32_t)pOverlayLookup[4];   // case 8 -> [4]
        case 9: return (uint32_t)pOverlayLookup[5];   // case 9 -> [5]
        default:
            // matches disasm path: PUSH 0x274 -> CleanupAndAbort
            // shadow: never reached in normal gameplay; emit sentinel
            return 0u;
    }
}
