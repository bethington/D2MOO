// D2MOO_REIMPL_EXPORT: ITEMS_GetRandomInRangeFromUnitEx_64d0
#include "../provider_runtime.h"
#include <stdlib.h>

// Register ABI: EAX=nMin, ECX=nMax, EDX=pUnit
// RET imm 0x0 (no stack args)
extern "C" int __cdecl ITEMS_GetRandomInRangeFromUnitEx_64d0(
    int nMin,    // EAX
    int nMax,    // ECX
    void* pUnit  // EDX
) {
    // 1) nMin == nMax short-circuit
    if (nMin == nMax) {
        return nMax;
    }

    // 2) Establish maxVal/minVal (post-swap semantics)
    int maxVal;
    int minVal;
    if (nMin >= nMax) {
        maxVal = nMin;
        minVal = nMax;
    } else {
        maxVal = nMax;
        minVal = nMin;
    }

    // 3) Validate pUnit (CleanupAndAbort 0x1505 path -> _exit)
    if (pUnit == (void*)0x0) {
        _exit(-1);
    }

    DWORD* pUnitDwords = (DWORD*)pUnit;
    DWORD dwType = pUnitDwords[0];        // offset 0x00

    // 4) dwType must be ITEM (4) (CleanupAndAbort 0x1506 path)
    if (dwType != 4) {
        _exit(-1);
    }

    // 5) Fetch pItemData at offset 0x14 (CleanupAndAbort 0x1508 path)
    BYTE* pItemData = *(BYTE**)(pUnitDwords + 5);  // [EDX + 0x14]
    if (pItemData == (BYTE*)0x0) {
        _exit(-1);
    }

    // 6) Initial range = maxVal - minVal
    uint32_t range = (uint32_t)(maxVal - minVal);

    // 7) Boundary flag at pItemData + 0x30 (word); non-zero -> range+1
    uint16_t boundaryFlag = *(uint16_t*)(pItemData + 0x30);
    if (boundaryFlag != 0) {
        range = range + 1;
    }

    // 8) Seed pointer computation: pItemData + dwType (dwType==4 here)
    DWORD* pSeed;
    if (dwType == 4 && pItemData != (BYTE*)0x0) {
        pSeed = (DWORD*)((BYTE*)pItemData + (DWORD)dwType);  // +0x04
    } else {
        pSeed = (DWORD*)0x0;
    }

    // 9) range <= 0 -> return minVal
    if ((int)range <= 0) {
        return minVal;
    }

    // 10) LCG: state = state * 0x6AC690C5 + seed_hi  (64-bit)
    DWORD seed_lo = pSeed[0];
    DWORD seed_hi = pSeed[1];

    uint64_t product     = (uint64_t)seed_lo * (uint64_t)0x6AC690C5ULL;
    uint64_t new_state   = product + (uint64_t)seed_hi;
    uint32_t       new_lo      = (uint32_t)new_state;
    uint32_t       new_hi      = (uint32_t)(new_state >> 32);

    // Store high part first, low part second (matches assembly ordering)
    pSeed[1] = new_hi;
    pSeed[0] = new_lo;

    // 11) Power-of-2 fast path uses bitmask, otherwise modulo
    uint32_t result;
    if ((range & (range - 1)) == 0) {
        // Power-of-2 path: (new_lo & (range-1)) + minVal
        uint32_t masked = new_lo & (range - 1);
        result = masked + (uint32_t)minVal;
    } else {
        // General path: (new_lo % range) + minVal
        uint32_t remainder = new_lo % range;
        result = remainder + (uint32_t)minVal;
    }

    return (int)result;
}
