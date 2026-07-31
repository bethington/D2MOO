// D2MOO_REIMPL_EXPORT: InitializeSmackVideoFromMemory
#include "../provider_runtime.h"

// Storm.dll / Win32 helpers used by this wrapper
extern "C" char*  __cdecl  SStrCopy(char* dest, const char* src, int maxlen);
extern "C" uint32_t __stdcall GetTempPathA(uint32_t nLen, char* lpBuf);
extern "C" uint32_t   __stdcall GetTempFileNameA(const char* lpPath, const char* lpPrefix, uint32_t uUnique, char* lpName);
extern "C" int    __stdcall InitializeSmackVideoPlayer(void* pRes, int nWidth, void* pPal, void* pScr, int* pFrameFmt, uint32_t dwFlags, void** ppEnt);

// Internal Smack thunks (loaded from DAT_0004d768 etc. in original)
extern "C" void* __cdecl SmkThunk_0004d768();
extern "C" void  __cdecl SmkThunk_0004d794();
extern "C" void  __cdecl SmkThunk_0004d906(void*, int, int, int);
extern "C" void  __cdecl SmkThunk_0004d7ea(void*);
extern "C" void  __cdecl SmkThunk_0004dafc(void*);

extern "C" int __stdcall InitializeSmackVideoFromMemory(
    void*        pVideoData,
    uint32_t dwDataSize,
    int          nWidth,
    int*         pHeight,
    int*         pFrameRate,
    int*         pFrameFormat,
    uint32_t dwFlags,
    int*         pSmackHandle)
{
    // Zero the out-handle up front, exactly like original
    if (pSmackHandle != (int*)0) {
        *(int*)pSmackHandle = 0;
    }

    // Validation masks — bitwise AND; ALL must be 0xFFFFFFFF to pass
    uint32_t uVar2;
    if ((nWidth == 0) && ((dwFlags & 0x20000000u) != 0)) {
        uVar2 = 0u;
    } else {
        uVar2 = 0xFFFFFFFFu;
    }

    uint32_t uVar3;
    if ((pFrameRate == (int*)0) && (nWidth != 0)) {
        uVar3 = 0u;
    } else {
        uVar3 = 0xFFFFFFFFu;
    }

    uint32_t uVar4;
    if ((pFrameFormat == (int*)0) || (*pFrameFormat == 0xc)) {
        uVar4 = 0xFFFFFFFFu;
    } else {
        uVar4 = 0u;
    }

    uint32_t mask =
        (uint32_t)(-(int)(pVideoData != (void*)0 && dwDataSize != 0))
        & uVar2 & uVar3 & uVar4;

    if (mask == 0) {
        // Error path: original sets _g_dwLastError = 0x57 then invokes a thunk
        // abort/handler. Oracle only exercises valid in-range inputs.
        // NEEDS GLOBAL: g_dwLastError   (write 0x57)
        return 0;
    }

    // ---- Main path: build temp file, alloc resolution, dispatch to player ----
    char szTempPath[260] = {0};   // local_104 buffer (>=0x104 bytes for safety)
    char szTempFile[260] = {0};   // local_208 buffer
    char szDotSep[16]   = {0};    // g_szDotSep8 placeholder
    // NEEDS GLOBAL: g_szDotSep8

    SStrCopy(szTempPath, szDotSep, 0x7fffffff);
    szTempFile[0] = '\0';
    GetTempPathA(0x104u, szTempPath);
    GetTempFileNameA(szTempPath, "Vid", 0u, szTempFile);

    void* pResolution = SmkThunk_0004d768();
    if (pResolution == (void*)0xFFFFFFFFu) {
        return 0;
    }

    SmkThunk_0004d794();
    SmkThunk_0004d906(pResolution, 0, 0, 0);

    // locals pSStack_30 / pSStack_2c / ppSStack_20 are stack-resident, uninitialised
    void*  pPal  = (void*)0;
    void*  pScr  = (void*)0;
    void** ppEnt = (void**)0;

    int iVar1 = InitializeSmackVideoPlayer(
        pResolution,
        nWidth,
        pPal,
        pScr,
        pFrameFormat,
        dwFlags | 0x30000u,
        ppEnt);

    SmkThunk_0004d7ea(pResolution);
    SmkThunk_0004dafc((void*)0);   // &pSStack_24c cleanup thunk

    return iVar1;
}
