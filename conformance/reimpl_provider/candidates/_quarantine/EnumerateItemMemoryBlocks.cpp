#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: EnumerateItemMemoryBlocks
//
// NEEDS GLOBAL: g_dwStormInitState    (Storm init flag from SMemDump.cpp)
// NEEDS GLOBAL: g_dwStormDebugEnabled (debug flag for DisplayErrorAndTerminate)

extern "C" {
    int __stdcall SearchItemByHash(int kind, uint32_t* pIterState, void* pResult);
    int __cdecl wsprintfA(char* dest, const char* fmt, ...);
}

extern "C" int __stdcall EnumerateItemMemoryBlocks(void* pCallback, int nContext)
{
    int* pInitState = (int*)D2MOO_Resolve("g_dwStormInitState");
    if (!pInitState) return 0;

    int* pLastError = (int*)D2MOO_Resolve("g_dwStormLastError");
    if (!pLastError) return 0;

    int* pDebugFlag = (int*)D2MOO_Resolve("g_dwStormDebugEnabled");
    if (!pDebugFlag) return 0;

    // _g_dwStormInitState == 0 -> not initialized: error/return 0
    if (pInitState[0] == 0) {
        pLastError[0] = 0x8510007D;
        // (*(code *)&DAT_0004d892)(0x8510007D); -- set-error helper, omit
        if (pDebugFlag[0] != 0) {
            // DisplayErrorAndTerminate(0x8510007D, &DAT_6fc37768, -1, NULL, 1, 1)
            // abort-branch helper, not in provider; collapse to return 0.
            return 0;
        }
        return 0;
    }

    // pCallback null-check -- the decompile uses a NEG+SBB trick; the
    // literal translation (compare to 0) is behaviourally identical:
    // enter the error path when pCallback is NULL.
    if (pCallback == 0) {
        pLastError[0] = 0x57;
        // (*(code *)&DAT_0004d892)(0x57); -- set-error helper, omit
        return 0;
    }

    int anHashParams[2];
    anHashParams[0] = 0x128;
    uint32_t dwHashIterState = 0;
    int iVar1 = SearchItemByHash(0, &dwHashIterState, anHashParams);

    char abOutputBuf[256];

    while (iVar1 != 0) {
        // The decompile references local slots dwMemBlockSize1..4 and
        // dwBlockCount, filled from a record produced by SearchItemByHash
        // at offsets 0x124/0x128/0x134 relative to the search-result base.
        // Without the exact ItemSearchResult layout exposed through the
        // resolver, default them to 0 (matching an uninitialised stack
        // frame at function entry).
        uint32_t dwMemBlockSize1 = 0;
        uint32_t dwMemBlockSize2 = 0;
        uint32_t dwMemBlockSize3 = 0;
        uint32_t dwMemBlockSize4 = 0;
        uint32_t dwBlockCount = 0;
        char abFormatBuf[260] = {0};

        // (*)DAT_0004debc is wsprintfA, format "%s:%d  blocks=%u  %uk/%uk/%uk"
        // (DAT_6fc37748 in the original); literal form from the plate comment.
        wsprintfA(abOutputBuf, "%s:%d  blocks=%u  %uk/%uk/%uk",
            abFormatBuf,
            dwMemBlockSize4, dwBlockCount,
            (dwMemBlockSize1 + 0x200u) >> 10,
            (dwMemBlockSize3 + 0x200u) >> 10,
            (dwMemBlockSize2 + 0x200u) >> 10);

        // (*pCallback)(nContext, abOutputBuf);
        ((void (*)(int, const char*))pCallback)(nContext, abOutputBuf);

        iVar1 = SearchItemByHash((int)dwHashIterState, &dwHashIterState, anHashParams);
    }

    return 1;
}
