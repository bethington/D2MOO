#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STORM_InitializePaletteColorDistance
extern "C" void* __stdcall MEMORY_AllocateMemoryFromArena(uint32_t size, const char* sourceFile, uint32_t line, uint32_t flags);

extern "C" int __stdcall STORM_InitializePaletteColorDistance(
    void* pVideoEntity,
    void* pPaletteEntries,
    uint32_t dwStartIndex,
    uint32_t dwEndIndex)
{
    // NEEDS GLOBAL: g_bPaletteDistanceInit
    // NEEDS GLOBAL: g_anSquaredDist
    // NEEDS GLOBAL: g_adColorSearchOffsets0
    // NEEDS GLOBAL: g_dwColorSearchOffsets1
    // NEEDS GLOBAL: g_adColorSearchOffsets2
    // NEEDS GLOBAL: g_pSVIDSourceFile

    char* pBInit = (char*)D2MOO_Resolve("g_bPaletteDistanceInit");
    int* pSqDist = (int*)D2MOO_Resolve("g_anSquaredDist");
    int* pOff0 = (int*)D2MOO_Resolve("g_adColorSearchOffsets0");
    int* pOff1 = (int*)D2MOO_Resolve("g_dwColorSearchOffsets1");
    int* pOff2 = (int*)D2MOO_Resolve("g_adColorSearchOffsets2");
    const char* pSrc = (const char*)D2MOO_Resolve("g_pSVIDSourceFile");
    if (!pBInit || !pSqDist || !pOff0 || !pOff1 || !pOff2 || !pSrc) return 0;

    uint8_t* pbPal = (uint8_t*)pPaletteEntries;
    void** pVF = (void**)pVideoEntity;
    void* pBuf = pVF[0];

    if (pBuf == 0) {
        void* pv = MEMORY_AllocateMemoryFromArena(0x10000, pSrc, 0x8f, 8);
        pVF[0] = pv;
        pVF[1] = pv;
        pBuf = pv;
    }

    if (*pBInit == 0) {
        *pBInit = 1;
        for (int i = -0xff; i < 0x100; i++) {
            *(int*)((char*)pSqDist + i * 4) = i * i;
        }
    }

    int aiRed[256] = {0}, aiGrn[256] = {0}, aiBlu[256] = {0};
    if (dwStartIndex <= dwEndIndex) {
        uint8_t* pb = pbPal + 2 + dwStartIndex * 4;
        uint32_t u = dwStartIndex;
        do {
            aiRed[u] = (int)((char*)pSqDist + pb[-2] * 4);
            aiBlu[u] = (int)((char*)pSqDist + pb[-1] * 4);
            aiGrn[u] = (int)((char*)pSqDist + (*pb) * 4);
            u++; pb += 4;
        } while (u <= dwEndIndex);
    }

    int anHash[5864] = {0};

    int aiChain[512] = {0};
    if (dwStartIndex <= dwEndIndex) {
        int* pi = &aiChain[dwStartIndex * 2];
        uint8_t* lpsz = pbPal - 3 + dwStartIndex * 4;
        uint32_t u = dwStartIndex;
        do {
            if (u == 0 || lpsz[3] != lpsz[-1] || lpsz[4] != lpsz[0] || lpsz[5] != lpsz[1]) {
                int h = (lpsz[5] >> 4) + 1 + ((lpsz[4] >> 4) + 1 + ((lpsz[3] >> 4) + 1) * 0x12) * 0x12;
                int prev = anHash[h];
                ((char*)(pi + 1))[0] = (char)u;
                *pi = prev;
                anHash[h] = (int)pi;
            }
            u++; lpsz += 4; pi += 2;
        } while (u <= dwEndIndex);
    }

    if (dwStartIndex <= dwEndIndex) {
        int i3 = dwStartIndex * 0x101;
        uint8_t* pb = pbPal + 2 + dwStartIndex * 4;
        uint32_t u = dwStartIndex;
        uint8_t* pb16 = pb;
        do {
            if (dwStartIndex < u) {
                int nRow = dwStartIndex << 8;
                uint8_t* pb10 = pb;
                uint32_t dwRow = dwStartIndex;
                do {
                    uint8_t bBest = (uint8_t)u;
                    if (u <= dwRow) bBest = (uint8_t)dwRow;
                    uint32_t u5 = (pb10[-1] + pb16[-1]) >> 1;
                    uint32_t u14 = (pb10[-2] + pb16[-2]) >> 1;
                    uint32_t u15 = (*pb16 + *pb10) >> 1;
                    int i13 = ((pb10[-2] + pb16[-2]) >> 5) + 1;
                    int i11 = ((pb10[-1] + pb16[-1]) >> 5) + 1;
                    int i9 = ((*pb16 + *pb10) >> 5) + 1;
                    int* pi = (int*)anHash[i9 + (i11 + i13 * 0x12) * 0x12];
                    uint32_t dwBest = 0xffffffff;
                    if (pi == 0) {
                        uint8_t bAlt = 0;
                        do {
                            if (dwBest < 0x301) break;
                            int altIdx = bAlt * 12;
                            int sIdx = pOff2[bAlt * 3] + (pOff1[altIdx] + (pOff0[altIdx] + i13) * 0x12 + i11) * 0x12 + i9;
                            int* pu = (int*)anHash[sIdx];
                            while (pu != 0) {
                                uint32_t idx = ((uint8_t*)(pu + 1))[0];
                                uint32_t u6 = *(int*)((char*)aiGrn[idx] - u15 * 4) + *(int*)((char*)aiBlu[idx] - u5 * 4) + *(int*)((char*)aiRed[idx] - u14 * 4);
                                if (u6 < dwBest) { bBest = (uint8_t)idx; dwBest = u6; }
                                pu = (int*)*pu;
                            }
                            bAlt++;
                        } while (bAlt < 0x1a);
                    } else if (*pi == 0) {
                        bBest = ((uint8_t*)(pi + 1))[0];
                    } else {
                        do {
                            uint32_t idx = ((uint8_t*)(pi + 1))[0];
                            uint32_t u6 = *(int*)((char*)aiGrn[idx] - u15 * 4) + *(int*)((char*)aiBlu[idx] - u5 * 4) + *(int*)((char*)aiRed[idx] - u14 * 4);
                            if (u6 < dwBest) { bBest = (uint8_t)idx; dwBest = u6; }
                            pi = (int*)*pi;
                        } while (pi != 0);
                    }
                    ((uint8_t*)pBuf)[u * 0x100 + dwRow] = bBest;
                    ((uint8_t*)pBuf)[nRow + u] = bBest;
                    dwRow++; pb10 += 4; nRow += 0x100;
                } while (dwRow < u);
            }
            ((uint8_t*)pBuf)[i3] = (uint8_t)u;
            u++; pb16 += 4; i3 += 0x101;
        } while (u <= dwEndIndex);
    }

    return 1;
}
