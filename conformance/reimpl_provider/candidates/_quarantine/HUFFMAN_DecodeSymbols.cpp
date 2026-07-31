#include "../provider_runtime.h"

// NEEDS GLOBAL: HUFFMAN_DecodeNextCode
// NEEDS GLOBAL: HUFFMAN_RefillBuffer
extern "C" uint32_t __cdecl HUFFMAN_DecodeNextCode(void* pCtx, uint32_t* pBitPos, int nWidth);
extern "C" uint32_t __cdecl HUFFMAN_RefillBuffer(void* pCtx, int nCount);

// D2MOO_REIMPL_EXPORT: HUFFMAN_DecodeSymbols
extern "C" uint32_t __fastcall HUFFMAN_DecodeSymbols(
    void* pHuffCtx,            // EBX
    int nTileOffset,           // ESI
    int nTileWidth,            // EDI
    uint32_t dwBitPos,
    int nSymbolCount,
    uint8_t bShift,
    short* psDeltaBase)
{
    char* pHuffmanCtx = (char*)pHuffCtx;

    // Resolve g_pHuffSymbolTbl (pointer variable: deref once)
    // NEEDS GLOBAL: g_pHuffSymbolTbl
    void* symTblAddr = D2MOO_Resolve("g_pHuffSymbolTbl");
    if (!symTblAddr) return 0xFFFFFFFFu;
    uint32_t* symTbl = *(uint32_t**)symTblAddr;
    if (!symTbl) return 0xFFFFFFFFu;

    uint32_t uVar8 = 1u << (bShift & 0x1f);
    uint32_t uVar6 = (uint32_t)(-1 << (bShift & 0x1f));

    uint32_t _bShift = dwBitPos;
    uint32_t uVar4;

    if ((*(uint32_t*)(pHuffmanCtx + 0x10) == 0) && ((int)dwBitPos <= nSymbolCount))
    {
        do
        {
            uVar4 = HUFFMAN_DecodeNextCode(pHuffmanCtx, &dwBitPos, nTileWidth);
            if (uVar4 != 0)
            {
err_decodeReturn:
                // return uVar4 & ((-1 < (int)uVar4) - 1)
                int sVar = (int)uVar4;
                uint32_t mask = (sVar < 0) ? 0xFFFFFFFFu : 0u;
                return uVar4 & mask;
            }

            int nibbleGroup = (int)(dwBitPos >> 4);
            int nibbleIdx = (int)(dwBitPos & 0xf);

            if (nibbleIdx == 0)
            {
                if (nibbleGroup != 0xf)
                {
                    uint32_t uVar9 = 1u << ((uint32_t)nibbleGroup & 0x1f);
                    *(uint32_t*)(pHuffmanCtx + 0x10) = uVar9;
                    if (nibbleGroup != 0)
                    {
                        if ((*(int*)(pHuffmanCtx + 0x0c) < nibbleGroup) &&
                            (uVar4 = HUFFMAN_RefillBuffer(pHuffmanCtx, nibbleGroup), uVar4 != 0))
                        {
                            goto err_decodeReturn;
                        }
                        uint32_t hiBits = *(uint32_t*)(pHuffmanCtx + 4);
                        int nBits = *(int*)(pHuffmanCtx + 0x0c) - nibbleGroup;
                        *(int*)(pHuffmanCtx + 0x0c) = nBits;
                        uint64_t uVar10 = ((uint64_t)hiBits) >> ((uint64_t)(nBits & 0x3F));
                        *(uint32_t*)(pHuffmanCtx + 0x10) += (uint32_t)uVar10 & (uVar9 - 1);
                    }
                    break;
                }
            }
            else
            {
                if ((*(int*)(pHuffmanCtx + 0x0c) < 1) &&
                    (uVar4 = HUFFMAN_RefillBuffer(pHuffmanCtx, 1), uVar4 != 0))
                    goto err_refillReturn;
                int nBits = *(int*)(pHuffmanCtx + 0x0c) + (-1);
                *(int*)(pHuffmanCtx + 0x0c) = nBits;
                uint32_t hiBits = *(uint32_t*)(pHuffmanCtx + 4);
                uint64_t uVar10 = ((uint64_t)hiBits) >> ((uint64_t)(nBits & 0x3F));
                nibbleIdx = (int)uVar6;
                if ((uVar10 & 1ULL) != 0)
                {
                    nibbleIdx = (int)uVar8;
                }
            }

            // Inner loop over symbol indices
            uint32_t* ppuVar7 = symTbl + _bShift;
            int zeroCount = nibbleGroup;
            do
            {
                uint32_t symIdx = *ppuVar7;
                short* psVar1 = (short*)(nTileOffset + (int)symIdx * 2);
                if (*psVar1 == 0)
                {
                    // Ghidra trick: pHuffCtx[-1].dwField3C + 3 == pHuffCtx - 1 (assuming sizeof=0x40)
                    zeroCount--;
                    if (zeroCount < 0) break;
                }
                else
                {
                    if ((*(int*)(pHuffmanCtx + 0x0c) < 1) &&
                        (uVar4 = HUFFMAN_RefillBuffer(pHuffmanCtx, 1), uVar4 != 0))
                        goto err_refillReturn;
                    int nBits = *(int*)(pHuffmanCtx + 0x0c) + (-1);
                    *(int*)(pHuffmanCtx + 0x0c) = nBits;
                    uint32_t hiBits = *(uint32_t*)(pHuffmanCtx + 4);
                    uint64_t uVar10 = ((uint64_t)hiBits) >> ((uint64_t)(nBits & 0x3F));
                    if (((uVar10 & 1ULL) != 0))
                    {
                        short sVar2 = *psVar1;
                        if ((uVar8 & (int)sVar2) == 0)
                        {
                            int iVal5;
                            if (sVar2 < 0)
                                iVal5 = (int)psDeltaBase[symIdx] * (int)uVar6;
                            else
                                iVal5 = (int)psDeltaBase[symIdx] * (int)uVar8;
                            *psVar1 = (short)((iVal5 + 0x40) >> 7) + sVar2;
                        }
                    }
                }
                _bShift = _bShift + 1;
                ppuVar7 = ppuVar7 + 1;
            } while ((int)_bShift <= nSymbolCount);

            if (nibbleIdx != 0)
            {
                uint32_t symIdx = symTbl[_bShift];
                short* psVar1 = (short*)(nTileOffset + (int)symIdx * 2);
                *psVar1 = (short)(((int)psDeltaBase[symIdx] * nibbleIdx + 0x40) >> 7);
            }
            _bShift = _bShift + 1;
        } while ((int)_bShift <= nSymbolCount);
    }

    // Process remaining symbols if dwField28 > 0
    if ((int)*(uint32_t*)(pHuffmanCtx + 0x10) > 0)
    {
        if ((int)_bShift <= nSymbolCount)
        {
            uint32_t* ppuVar7 = symTbl + _bShift;
            do
            {
                uint32_t symIdx = *ppuVar7;
                short* psVar1 = (short*)(nTileOffset + (int)symIdx * 2);
                if (*psVar1 != 0)
                {
                    if ((*(int*)(pHuffmanCtx + 0x0c) < 1) &&
                        (uVar4 = HUFFMAN_RefillBuffer(pHuffmanCtx, 1), uVar4 != 0))
                    {
err_refillReturn:
                        int sVar = (int)uVar4;
                        uint32_t mask = (sVar < 0) ? 0xFFFFFFFFu : 0u;
                        return uVar4 & mask;
                    }
                    int nBits = *(int*)(pHuffmanCtx + 0x0c) + (-1);
                    *(int*)(pHuffmanCtx + 0x0c) = nBits;
                    uint32_t hiBits = *(uint32_t*)(pHuffmanCtx + 4);
                    uint64_t uVar10 = ((uint64_t)hiBits) >> ((uint64_t)(nBits & 0x3F));
                    if (((uVar10 & 1ULL) != 0))
                    {
                        short sVar2 = *psVar1;
                        if ((uVar8 & (int)sVar2) == 0)
                        {
                            int iVal5;
                            if (sVar2 < 0)
                                iVal5 = (int)psDeltaBase[symIdx] * (int)uVar6;
                            else
                                iVal5 = (int)psDeltaBase[symIdx] * (int)uVar8;
                            *psVar1 = (short)((iVal5 + 0x40) >> 7) + sVar2;
                        }
                    }
                }
                _bShift = _bShift + 1;
                ppuVar7 = ppuVar7 + 1;
            } while ((int)_bShift <= nSymbolCount);
        }
        *(uint32_t*)(pHuffmanCtx + 0x10) = *(uint32_t*)(pHuffmanCtx + 0x10) - 1;
    }

    return 0;
}
