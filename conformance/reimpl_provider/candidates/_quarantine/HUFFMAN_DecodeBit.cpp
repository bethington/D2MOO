#include "../provider_runtime.h"

// Forward declare the refill helper called by the original
extern "C" uint32_t __fastcall HUFFMAN_RefillBuffer(void* pCtx, int nBits);

// D2MOO_REIMPL_EXPORT: HUFFMAN_DecodeBit
extern "C" uint32_t __fastcall HUFFMAN_DecodeBit(
    void* pHuffmanCtx,           // ESI - HuffmanDecodeCtxLarge*
    void* pOutValue,              // EDI - ushort*
    uint8_t bBitPosition,   // stack [ESP+8]
    short nSetValue               // stack [ESP+12]
)
{
    if (!pHuffmanCtx || !pOutValue)
        return 0xFFFFFFFFu;

    int nBitsInBuffer = *(int*)((char*)pHuffmanCtx + 0x08);

    if (nBitsInBuffer < 1) {
        uint32_t dwResult = HUFFMAN_RefillBuffer(pHuffmanCtx, 1);
        if (dwResult != 0) {
            return dwResult & ((-1 < (int)dwResult) - 1);
        }
    }

    int nBitIndex = nBitsInBuffer + -1;
    *(int*)((char*)pHuffmanCtx + 0x08) = nBitIndex;

    uint32_t highDword = *(uint32_t*)((char*)pHuffmanCtx + 0x04);
    uint64_t qwBitValue = (uint64_t)highDword >> nBitIndex;

    if ((qwBitValue & 1ULL) != 0) {
        uint16_t mask = (uint16_t)(((int)nSetValue * (1 << (bBitPosition & 0x1f)) + 0x40) >> 7);
        *(uint16_t*)pOutValue = (uint16_t)(*(uint16_t*)pOutValue | mask);
    }

    return 0;
}
