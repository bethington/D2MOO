// D2MOO_REIMPL_EXPORT: STORM_SetupDeltaEncodingParams
// Reimpl provider version of Storm.dll!0x6fc11b30.
// Reads NO global state (all inputs come via fastcall args + stack args).
// Resolves no globals via D2MOO_Resolve -- no global symbol needed.
#include "../provider_runtime.h"

// Subroutine declared by the decompile (Storm.dll helper).
// __fastcall: ECX=pEncodingParams, EDX=bufferSize, then 3 stack args.
extern "C" void* __fastcall EncodeShortArrayWithDeltaBits(
    void*  pEncodingParams,   // ECX -- short* source array
    uint32_t bufferSize,  // EDX -- buffer capacity
    void*  pBitWidthTable,    // stack[0x4]
    int    someParam,         // stack[0x8] (literal 2 in decompile)
    uint8_t valueBits); // stack[0xc]

// D2MOO_REIMPL_EXPORT: STORM_SetupDeltaEncodingParams
// __fastcall: ECX=pEncodingParams, EDX=pOutputSize, RET 0x10 cleans 4 stack args.
// dwNumChannels (last stack arg) selects precision (valueBits / deltaBits).
extern "C" void* __fastcall STORM_SetupDeltaEncodingParams(
    void*        pEncodingParams,        // ECX  -- short* source array
    uint32_t* pOutputSize,          // EDX  -- uint* (in:buf size, out:encoded end ptr)
    void*        pBitWidthTable,        // stack[0x4] -- short* bounds
    uint32_t unused_dwNumChannels,  // stack[0x8] -- ignored
    void*        pStateParams,          // stack[0xc] -- receives deltaBits dword
    uint32_t dwNumChannels)         // stack[0x10] -- channel-count selector
{
    uint8_t valueBits;
    uint8_t deltaBits;

    if (dwNumChannels != 0) {
        if (dwNumChannels < 3) {
            // 1-2 channels: low precision
            valueBits = 4;
            deltaBits = 6;
        } else if (dwNumChannels == 3) {
            // 3 channels: high precision
            valueBits = 6;
            deltaBits = 8;
        } else {
            // > 3 channels: standard precision
            valueBits = 5;
            deltaBits = 7;
        }
    } else {
        // 0 channels: standard precision
        valueBits = 5;
        deltaBits = 7;
    }

    // Decompile emits *(undefined4 *)pStateParams = deltaBits;  (dword write, low byte = deltaBits)
    *(uint32_t*)pStateParams = (uint32_t)deltaBits;

    // Dispatch to encoder with selected precision; 5th arg = (byte)dwNumChannels in original.
    void* pbEncodedEnd = EncodeShortArrayWithDeltaBits(
        pEncodingParams,
        *pOutputSize,
        pBitWidthTable,
        2,
        valueBits);

    *pOutputSize = (uint32_t)pbEncodedEnd;
    return pbEncodedEnd;
}
