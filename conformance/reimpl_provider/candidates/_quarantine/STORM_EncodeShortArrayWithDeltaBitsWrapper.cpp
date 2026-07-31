#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STORM_EncodeShortArrayWithDeltaBitsWrapper

// Inner encoder called by this wrapper (Storm.dll).
// Decompile call site:
//   EncodeShortArrayWithDeltaBits(pEncodingParams, *pOutputSize,
//                                 pBitWidthTable, 1, (byte)bitWidth) -> void*
extern "C" void* __cdecl EncodeShortArrayWithDeltaBits(
    short* pEncodingParams,
    uint32_t outputSize,
    short* pBitWidthTable,
    uint32_t dwCompressionLevel,
    uint8_t bitWidth);

extern "C" void* __fastcall STORM_EncodeShortArrayWithDeltaBitsWrapper(
    short* pEncodingParams,            // ECX: input array to encode
    uint32_t* pOutputSize,         // EDX: in/out (initial size -> encoded pointer)
    short* pBitWidthTable,             // stack[0]
    uint32_t dwCompressionLevel,   // stack[1] (ignored; wrapper hardcodes 1)
    void* pStateParams,                // stack[2]: outbuf, written as 4-byte dword
    uint32_t dwNumChannels         // stack[3]
) {
    (void)dwCompressionLevel; // wrapper always passes literal 1 to inner encoder

    // Channel-count-driven state selection (mirrors the decompile flow).
    // The decompile has NO early-exit on dwNumChannels==0; that case falls
    // through to the default branch (bitWidth=5, state=7).
    uint8_t bitWidth;
    uint32_t state;
    if (dwNumChannels != 0) {
        if (dwNumChannels < 3) {
            bitWidth = 4;
            state = 6;
        } else if (dwNumChannels == 3) {
            bitWidth = 6;
            state = 8;
        } else {
            bitWidth = 5;
            state = 7;
        }
    } else {
        bitWidth = 5;
        state = 7;
    }
    // Decompile writes a 4-byte dword (per *(undefined4*)pStateParams = state).
    *(int*)pStateParams = (int)state;

    void* pbEncoded = EncodeShortArrayWithDeltaBits(
        pEncodingParams, *pOutputSize, pBitWidthTable, 1u, bitWidth);
    *pOutputSize = (uint32_t)pbEncoded;
    return pbEncoded;
}
