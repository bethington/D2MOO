#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: BroadcastGameCommand

// NEEDS GLOBAL: g_dwLastProcessedTick
// NEEDS GLOBAL: g_dwMsPerFrame
// NEEDS GLOBAL: g_pDataSyncClientHashTable
// NEEDS GLOBAL: g_dwBypassFpsThrottle

extern "C" int __stdcall QueryPerformanceCounter(uint64_t* lpPerformanceCount);
extern "C" uint32_t __stdcall timeGetTime();
extern "C" void* __stdcall AcquireGameSession();
extern "C" void __stdcall GAME_ProcessGameFrameTick();
extern "C" void __stdcall LeaveCriticalSection(void* lpCriticalSection);

extern "C" int __stdcall BroadcastGameCommand(int nCommandType)
{
    char* baseLastProcessedTick = (char*)D2MOO_Resolve("g_dwLastProcessedTick");
    char* baseMsPerFrame = (char*)D2MOO_Resolve("g_dwMsPerFrame");
    char* baseHashTable = (char*)*(void**)D2MOO_Resolve("g_pDataSyncClientHashTable");
    char* baseBypassFpsThrottle = (char*)D2MOO_Resolve("g_dwBypassFpsThrottle");

    if (!baseLastProcessedTick || !baseMsPerFrame || !baseHashTable || !baseBypassFpsThrottle)
        return (int)0xDEADBEEFu;

    uint32_t dwTimeNow;
    uint32_t dwDeltaMs;
    uint32_t* pGameSession;
    uint32_t uVar1;
    uint32_t* pSlotCritSections;
    uint32_t dwCurrentTick;
    int fProcessed;
    uint64_t perfStart = 0;
    uint64_t perfEnd = 0;

    fProcessed = 0;
    QueryPerformanceCounter(&perfStart);
    dwTimeNow = timeGetTime();
    dwCurrentTick = dwTimeNow & 0x7fffffff;

    if (*(uint32_t*)baseLastProcessedTick == 0) {
        *(uint32_t*)baseLastProcessedTick = dwCurrentTick;
    }
    if ((int)(dwCurrentTick - *(uint32_t*)baseLastProcessedTick) < (int)*(uint32_t*)baseMsPerFrame) {
        return 0;
    }
    dwDeltaMs = (dwCurrentTick - *(uint32_t*)baseLastProcessedTick) - *(uint32_t*)baseMsPerFrame;
    if ((nCommandType != 0) && (*(uint32_t*)baseMsPerFrame <= dwDeltaMs)) {
        dwDeltaMs = *(uint32_t*)baseMsPerFrame;
    }
    *(uint32_t*)baseLastProcessedTick = dwCurrentTick - dwDeltaMs;

    pSlotCritSections = (uint32_t*)(baseHashTable + 0x14);
    while (pSlotCritSections < (uint32_t*)(baseHashTable + 0x1014)) {
        if ((*pSlotCritSections != 0) && (*pSlotCritSections != 0xffffffffu)) {
            pGameSession = (uint32_t*)AcquireGameSession();
            if (pGameSession != 0) {
                fProcessed = 1;
                if (*(uint32_t*)baseBypassFpsThrottle != 0) {
                    pGameSession[0x76f] = 0x400;
                    goto skip_frame_processing;
                }
                if (pGameSession[0x76e] != 0) {
                    uVar1 = ((dwCurrentTick - pGameSession[0x76e]) * 0x400u) / *(uint32_t*)baseMsPerFrame;
                    pGameSession[0x76f] = uVar1;
                    if (uVar1 < 10) {
                        pGameSession[0x76f] = 10;
                        pGameSession[0x76e] = dwCurrentTick;
                        goto skip_frame_processing;
                    }
                    if (0x800 < uVar1) {
                        pGameSession[0x76f] = 0x800;
                    }
                }
                pGameSession[0x76e] = dwCurrentTick;
skip_frame_processing:
                GAME_ProcessGameFrameTick();
                if ((void*)pGameSession[6] == (void*)0x0) {
                    return 0;
                }
                LeaveCriticalSection((void*)pGameSession[6]);
            }
        }
        pSlotCritSections = pSlotCritSections + 1;
    }

    if (fProcessed) {
        QueryPerformanceCounter(&perfEnd);
        return (int)((uint32_t)perfEnd - (uint32_t)perfStart);
    }
    return 0;
}
