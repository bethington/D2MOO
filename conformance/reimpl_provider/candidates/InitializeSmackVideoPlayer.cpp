#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitializeSmackVideoPlayer

// All needed Storm.dll globals are NOT in the resolvable list:
// NEEDS GLOBAL: g_dwLastError
// NEEDS GLOBAL: g_dwDisplayWidth
// NEEDS GLOBAL: g_dwDisplayHeight
// NEEDS GLOBAL: g_pDirectDrawSurface
// NEEDS GLOBAL: g_pfnSetLastError
// NEEDS GLOBAL: g_dwVideoDisplay
// NEEDS GLOBAL: g_hSmackLibrary
// NEEDS GLOBAL: g_pSmackFunctionTable
// NEEDS GLOBAL: g_pCritSecGameState
// NEEDS GLOBAL: g_pfnSmackVideoVtable
// NEEDS GLOBAL: g_pSVIDSourceFile
// NEEDS GLOBAL: g_dwWndPalette

// Stub functions for external calls (Windows API / Storm internal)
typedef void* (__stdcall *SmackSurfaceInitFn)(void*, uint32_t, uint32_t);
typedef void* (__stdcall *SmackAudioAllocFn)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef void (__stdcall *SmackAudioRenderFn)(void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

static uint32_t __stdcall stub_GetSystemMetrics(uint32_t idx) { return 0; }
static int __stdcall stub_SRegLoadValue(void* a, const char* b, uint32_t c, uint32_t* d) { return 0; }
static int __stdcall stub_InitializeAudioStreams(uint32_t a) { return 1; }
static void* __stdcall stub_AllocateVideoRecorder(void* vtable, uint32_t a, uint32_t b) { return 0; }
static void* __stdcall stub_AllocFromArena(uint32_t tag, const char* file, uint32_t size, uint32_t align) { return 0; }
static int __stdcall stub_SF_OpenAndValidateGameFile(char* a, char* b, uint32_t c, uint32_t* d) { return 0; }
static void __stdcall stub_DestroyGameEntity(void* ent) { }
static void* __stdcall stub_GetDC(void* h) { return 0; }
static void __stdcall stub_ReleaseDC(void* h, void* dc) { }
static void __stdcall stub_GetSystemPaletteEntries(void* dc, uint32_t a, uint32_t b, void* c) { }
static void __stdcall stub_InitPaletteColorDistance(void* ent, void* entries, uint32_t start, uint32_t end) { }
static int __stdcall stub_InitializeAndValidateSlot(uint32_t a) { return 1; }
static void __stdcall stub_CopyPaletteData(uint32_t idx, uint32_t count, void* data, uint32_t flag) { }

extern "C" int __stdcall InitializeSmackVideoPlayer(
    void* pResolution,
    int nDisplaySurfaceMode,
    void* pPaletteData,
    void* pScreenResolution,
    void* pVideoFlags,
    uint32_t dwFlags,
    void** ppVideoObject)
{
    if (ppVideoObject != 0) {
        *(void**)ppVideoObject = 0;
    }

    // Validation gate (parameter-only)
    uint32_t dwVideoFlags_v;
    if ((nDisplaySurfaceMode == 0) && ((dwFlags & 0x20000000u) != 0)) {
        dwVideoFlags_v = 0;
    } else {
        dwVideoFlags_v = 0xFFFFFFFFu;
    }

    uint32_t dwRenderMode_v;
    if ((pScreenResolution == 0) && (nDisplaySurfaceMode != 0)) {
        dwRenderMode_v = 0;
    } else {
        dwRenderMode_v = 0xFFFFFFFFu;
    }

    uint32_t dwValidationMask_v;
    if ((pVideoFlags == 0) || (*(uint32_t*)pVideoFlags == 0xCu)) {
        dwValidationMask_v = 0xFFFFFFFFu;
    } else {
        dwValidationMask_v = 0;
    }

    uint32_t cond = (pResolution != 0 && ppVideoObject != 0) ? 0xFFFFFFFFu : 0u;
    uint32_t gate = cond & dwVideoFlags_v & dwRenderMode_v & dwValidationMask_v;
    if (gate == 0) {
        return 0;
    }

    // Get display dimensions
    char* base_dwDisplayWidth = (char*)D2MOO_Resolve("g_dwDisplayWidth");
    char* base_dwDisplayHeight = (char*)D2MOO_Resolve("g_dwDisplayHeight");
    char* base_pDirectDrawSurface = (char*)D2MOO_Resolve("g_pDirectDrawSurface");
    if (!base_dwDisplayWidth || !base_dwDisplayHeight || !base_pDirectDrawSurface) return 0;

    uint32_t dwScreenWidth = *(uint32_t*)(base_dwDisplayWidth + 0);
    uint32_t pDDS_val = *(uint32_t*)(base_pDirectDrawSurface + 0);
    if (pDDS_val == 0) {
        dwScreenWidth = stub_GetSystemMetrics(0);
    }
    uint32_t dwScreenHeight;
    if (pDDS_val == 0) {
        dwScreenHeight = stub_GetSystemMetrics(1);
    } else {
        dwScreenHeight = *(uint32_t*)(base_dwDisplayHeight + 0);
    }

    // Video config parsing (0x808)
    uint32_t dwFlags_local = dwFlags;
    if ((dwFlags_local & 0x808u) != 0) {
        uint32_t dwConfigMode = 0;
        void* g_dwVideoDisplay = D2MOO_Resolve("g_dwVideoDisplay");
        if (g_dwVideoDisplay) {
            stub_SRegLoadValue(g_dwVideoDisplay, "Mode", 0, &dwConfigMode);
        }
        uint32_t uVar2 = dwConfigMode;
        if ((dwConfigMode & 0x100u) == 0) {
            uVar2 = 0x203u;
        }
        if ((dwFlags_local & 0x800u) != 0) {
            dwFlags_local = dwFlags_local ^ (uVar2 ^ dwFlags_local) & 0x300u;
        }
        if ((dwFlags_local & 8u) != 0) {
            dwFlags_local = dwFlags_local ^ (uVar2 ^ dwFlags_local) & 7u;
        }
    }

    // Audio system check
    char* base_hSmackLibrary = (char*)D2MOO_Resolve("g_hSmackLibrary");
    char* base_pSmackFunctionTable = (char*)D2MOO_Resolve("g_pSmackFunctionTable");
    char* base_pCritSecGameState = (char*)D2MOO_Resolve("g_pCritSecGameState");
    if (!base_hSmackLibrary || !base_pSmackFunctionTable || !base_pCritSecGameState) return 0;

    uint32_t hSmackLib = *(uint32_t*)(base_hSmackLibrary + 0);
    uint32_t pSmackFnTbl = *(uint32_t*)(base_pSmackFunctionTable + 0);
    uint32_t pCritSecGS = *(uint32_t*)(base_pCritSecGameState + 0);

    if ((hSmackLib == 0) || (pSmackFnTbl == 0) || (pCritSecGS == 0)) {
        int iVar3 = stub_InitializeAudioStreams(0);
        if (iVar3 == 0) return 0;
    }

    // Allocate video recorder
    void* base_pfnSmackVideoVtable = D2MOO_Resolve("g_pfnSmackVideoVtable");
    if (!base_pfnSmackVideoVtable) return 0;
    void* pEntity = stub_AllocateVideoRecorder(base_pfnSmackVideoVtable, 0, 0);
    if (!pEntity) return 0;

    // Apply mirror/rotation flags
    uint32_t uVar2_local = dwFlags_local ^ (~(dwFlags_local >> 1) ^ dwFlags_local) & 0x10000000u;
    *(uint32_t*)((char*)pEntity + 8) = 0xFFFFFFFFu;
    if ((uVar2_local & 0x20000000u) != 0) {
        uVar2_local = uVar2_local & 0xFFF7FFFFu;
    }
    if ((uVar2_local & 0x10000000u) != 0) {
        nDisplaySurfaceMode = 0;
    }
    *(uint32_t*)((char*)pEntity + 0x5C) = (uint32_t)nDisplaySurfaceMode;

    // Copy palette data
    if (pPaletteData != 0) {
        *(uint32_t*)((char*)pEntity + 0x60) = *(uint32_t*)((char*)pPaletteData + 0);
        *(uint32_t*)((char*)pEntity + 0x64) = *(uint32_t*)((char*)pPaletteData + 4);
        *(uint32_t*)((char*)pEntity + 0x68) = *(uint32_t*)((char*)pPaletteData + 8);
        *(uint32_t*)((char*)pEntity + 0x6C) = *(uint32_t*)((char*)pPaletteData + 12);
        *(uint32_t*)((char*)pEntity + 0x70) = (uint32_t)((char*)pEntity + 0x60);
    }

    // Copy screen resolution
    if (pScreenResolution != 0) {
        *(uint32_t*)((char*)pEntity + 0x74) = *(uint32_t*)((char*)pScreenResolution + 0);
        *(uint32_t*)((char*)pEntity + 0x78) = *(uint32_t*)((char*)pScreenResolution + 4);
    }

    // Copy video flags
    if (pVideoFlags == 0) {
        *(uint32_t*)((char*)pEntity + 0x50) = 1;
        *(uint32_t*)((char*)pEntity + 0x8C) = 0xFEu;
        *(uint32_t*)((char*)pEntity + 0x90) = 0;
        *(uint32_t*)((char*)pEntity + 0x94) = 0;
        *(uint32_t*)((char*)pEntity + 0x98) = 0;
    } else {
        *(uint32_t*)((char*)pEntity + 0x50) = *(uint32_t*)((char*)pVideoFlags + 4);
        *(uint32_t*)((char*)pEntity + 0x8C) = *(uint32_t*)((char*)pVideoFlags + 8);
    }

    // Allocate codec node
    char* base_pSVIDSourceFile = (char*)D2MOO_Resolve("g_pSVIDSourceFile");
    void* pCVar4 = stub_AllocFromArena(0xC, base_pSVIDSourceFile, 0x2A8, 8);
    *(void**)((char*)pEntity + 0x58) = pCVar4;

    // Calculate render flags
    uint32_t uVar6 = 0x1000u;
    uint32_t dwConfigMode_local = 0xFFFFFFFFu;
    if ((uVar2_local & 0x20000u) != 0) uVar6 = 0x1200u;
    if ((uVar2_local & 0x400000u) != 0) uVar6 = uVar6 | 0x400u;
    if ((uVar2_local & 0x1000000u) == 0) uVar6 = uVar6 | 0xFE000u;
    if ((uVar2_local & 0x2000000u) != 0) uVar6 = uVar6 | 0x20u;
    if ((uVar2_local & 0x4000000u) != 0) uVar6 = uVar6 | 0x40u;

    // File handle validation
    void* pResolution_local = pResolution;
    if ((uVar2_local & 0x10000u) == 0) {
        int fSuccess = stub_SF_OpenAndValidateGameFile(0, (char*)pResolution_local, 5, &dwConfigMode_local);
        if (fSuccess == 0) goto ErrorCleanup;
        *(uint32_t*)((char*)pEntity + 8) = dwConfigMode_local;
        pResolution_local = (void*)(uint32_t)dwConfigMode_local;
    }

    // Smack surface init
    if (!pSmackFnTbl) { stub_DestroyGameEntity(pEntity); return 0; }
    void* pCodecBuffer = ((SmackSurfaceInitFn)*(void**)((char*)pSmackFnTbl + 0x1C))(pResolution_local, uVar6, 0xFFFFFFFFu);
    *(void**)((char*)pCVar4 + 4) = pCodecBuffer;

    void* pCodecNodeNext = *(void**)((char*)pCVar4 + 4);
    if (pCodecNodeNext == 0) goto ErrorCleanup;

    // Validate dimensions
    int iVar3 = *(int*)((char*)pCodecNodeNext + 4) * 2;
    if ((iVar3 - (int)dwScreenWidth != 0) && ((int)dwScreenWidth <= iVar3)) {
        uVar2_local = uVar2_local & 0xFFFFFDFFu;
    }
    *(uint32_t*)((char*)pEntity + 0x84) = (uVar2_local >> 9) & 1;

    iVar3 = *(int*)((char*)pCodecNodeNext + 8) * 2;
    if ((iVar3 - (int)dwScreenHeight != 0) && ((int)dwScreenHeight <= iVar3)) {
        *(uint32_t*)((char*)pEntity + 0x84) = 0;
    }

    // Vertical offset
    if ((uVar2_local & 0x80000u) == 0) {
        *(uint32_t*)((char*)pCVar4 + 8) = 0;
    } else {
        uint32_t codecH = *(uint32_t*)((char*)pCodecNodeNext + 8);
        uint32_t scalingMode = *(uint32_t*)((char*)pEntity + 0x84);
        *(uint32_t*)((char*)pCVar4 + 8) = (dwScreenHeight - (scalingMode + 1) * codecH) >> 1;
    }

    // Audio interlace
    if ((uVar2_local & 0x200u) == 0) {
        *(uint32_t*)((char*)pEntity + 0x88) = 0;
    } else {
        uint32_t dwInterlaceMode;
        if ((*(uint32_t*)((char*)pEntity + 0x84) == 0) || ((uVar2_local & 1u) == 0) || ((uVar2_local & 2u) == 0)) {
            dwInterlaceMode = 0;
        } else {
            dwInterlaceMode = 1;
        }
        *(uint32_t*)((char*)pEntity + 0x88) = dwInterlaceMode;

        void* pvVar1 = *(void**)((char*)pEntity + 0x58);
        void* pvNext = *(void**)((char*)pvVar1 + 4);
        uint32_t uVar5 = *(uint32_t*)((char*)pvNext + 8);
        int audioBufSize = *(int*)((char*)pvNext + 4) * (dwInterlaceMode + 1);
        uint32_t dwAudioBufferSize = (uint32_t)((SmackAudioAllocFn)*(void**)((char*)pSmackFnTbl + 8))(0, 4, audioBufSize, uVar5, audioBufSize, uVar5);
        *(uint32_t*)pvVar1 = dwAudioBufferSize;

        int audioBufPtr = *(int*)pvVar1;
        if (audioBufPtr == 0) goto ErrorCleanup;

        void* pvVar1_2 = *(void**)((char*)pEntity + 0x58);
        void* pvNext_2 = *(void**)((char*)pvVar1_2 + 4);
        ((SmackAudioRenderFn)*(void**)((char*)pSmackFnTbl + 0x24))(pvNext_2, 0, 0,
            (*(uint32_t*)((char*)pEntity + 0x88) + 1) * *(int*)((char*)pvNext_2 + 4),
            *(uint32_t*)((char*)pvNext_2 + 8),
            *(uint32_t*)((int)audioBufPtr + 0x448), 0);
    }

    // Palette config
    uVar2_local = uVar2_local ^ (~(uVar2_local >> 1) ^ uVar2_local) & 0x100u;
    if ((uVar2_local & 0x100u) != 0) {
        uVar2_local = uVar2_local & 0xFFFFFFF8u;
    }
    *(uint32_t*)((char*)pEntity + 0x80) = uVar2_local;

    if (((uVar2_local & 0x100002u) == 0x100002u)) {
        char* base_dwWndPalette = (char*)D2MOO_Resolve("g_dwWndPalette");
        if (base_dwWndPalette) {
            uint32_t pWnd = *(uint32_t*)base_dwWndPalette;
            if (pWnd != 0) {
                void* hdc = stub_GetDC((void*)pWnd);
                stub_GetSystemPaletteEntries(hdc, 0, 0x100, &dwScreenHeight);
                stub_ReleaseDC((void*)pWnd, hdc);
                uint32_t uVar6_local;
                if ((uVar2_local & 0x800000u) == 0) {
                    uVar6_local = 0;
                } else {
                    char abPaletteBuffer[1024];
                    stub_InitPaletteColorDistance(pEntity, abPaletteBuffer, 0, 0x7F);
                    uVar6_local = 0x80u;
                }
                stub_InitPaletteColorDistance(pEntity, (void*)0, uVar6_local, 0xFF);
            }
        }
    }

    // Set surface init flag
    *(uint32_t*)((char*)pEntity + 0) = 0x40;
    *(void**)ppVideoObject = pEntity;

    // Slot system
    if ((uVar2_local & 0x200000u) != 0) {
        stub_InitializeAndValidateSlot(0);
    }

    // Default palette
    if ((pVideoFlags == 0) && ((uVar2_local & 0x100000u) == 0)) {
        uint32_t dwScreenWidth_local = 0;
        uint32_t dwConfigMode_local2 = 0xFFFFFFu;
        stub_CopyPaletteData(0, 1, &dwScreenWidth_local, 1);
        stub_CopyPaletteData(0xFF, 1, &dwConfigMode_local2, 1);
    }

    return 1;

ErrorCleanup:
    stub_DestroyGameEntity(pEntity);
    return 0;
}
