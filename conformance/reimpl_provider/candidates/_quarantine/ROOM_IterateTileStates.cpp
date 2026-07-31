#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ROOM_IterateTileStates

extern "C" int __cdecl ROOM_CheckTileAccessBoundary(void* pTileData, void* pTileAccessData, void* pCtx, uint32_t dwCol, uint32_t dwRow);
extern "C" void __cdecl ROOM_ResolveTileState(void* pCtx, void* pTileData, int* pLayerCount, uint32_t dwCol, uint32_t dwRow);
extern "C" void __cdecl ROOM_ApplyTileStateDelta(void* pCtx, void* pTileData, int* pLayerCount, uint32_t dwCol, uint32_t dwRow, uint32_t dwDelta);
extern "C" void __cdecl ANIM_InterpolateCelFrame(void* pData);
extern "C" void __cdecl ROOM_InterpolateTileData(void* pData);

// NEEDS GLOBAL: g_pfnReadTileData
// NEEDS GLOBAL: g_pfnClearTileData

extern "C" int __cdecl ROOM_IterateTileStates(
    int* pResult,
    uint8_t* pTileData,
    void* pTileAccessData,
    void* pCtx,
    uint32_t dwMode)
{
    // Resolve function pointer globals
    void* pReadTileDataAddr = D2MOO_Resolve("g_pfnReadTileData");
    if (!pReadTileDataAddr) return -0x7FFFFFFF;
    int (*pfnReadTileData)(uint8_t*, uint32_t, uint32_t, int*, int, uint32_t) = 
        *(int(**)(uint8_t*, uint32_t, uint32_t, int*, int, uint32_t))pReadTileDataAddr;
    if (!pfnReadTileData) return -0x7FFFFFFF;
    
    void* pClearTileDataAddr = D2MOO_Resolve("g_pfnClearTileData");
    if (!pClearTileDataAddr) return -0x7FFFFFFF;
    void (*pfnClearTileData)(void*) = *(void(**)(void*))pClearTileDataAddr;
    if (!pfnClearTileData) return -0x7FFFFFFF;
    
    // Local variables
    uint32_t uVar2;
    int iVar3;
    int iVar4;
    uint16_t* pTileData_00;
    uint32_t uVar5;
    int iVar6;
    uint32_t uVar7;
    int iVar8;
    int nTotalTileCount;
    uint32_t dwColCount;
    int nRowCount;
    int* pLayerIndex;
    int anLayerTileCount[4];
    uint32_t adLayerStride[4];
    uint32_t adLayerHeight[4];
    uint32_t adLayerWidth[4];
    
    // Step 1: Extract layer dimensions
    iVar8 = 0;
    iVar3 = *(int*)((char*)pTileAccessData + 0);
    iVar6 = 0;
    nTotalTileCount = 0;
    if (0 < iVar3) {
        uint32_t* puVar1 = (uint32_t*)(*(int*)((char*)pTileAccessData + 4) + 0xc);
        do {
            adLayerHeight[iVar6] = puVar1[1];
            adLayerWidth[iVar6] = puVar1[0];
            adLayerStride[iVar6] = *(uint32_t*)(puVar1[2] + 8);
            uVar2 = puVar1[-2];
            uVar5 = puVar1[-1];
            anLayerTileCount[iVar6] = uVar2 * uVar5;
            iVar8 = iVar8 + uVar2 * uVar5;
            iVar6 = iVar6 + 1;
            puVar1 = puVar1 + 6;
            nTotalTileCount = iVar8;
        } while (iVar6 < iVar3);
    }
    
    // Step 2: Calculate grid dimensions
    if (iVar3 == 1) {
        nTotalTileCount = 1;
        anLayerTileCount[0] = 1;
        uVar2 = (*(int*)((char*)pCtx + 0x37b8) << 3) / *(int*)(*(int*)((char*)pTileAccessData + 4) + 4);
        iVar3 = (*(int*)((char*)pCtx + 0x37bc) << 3) / *(int*)(*(int*)((char*)pTileAccessData + 4) + 8);
        dwColCount = (*(int*)((char*)pCtx + 0x4c) + -1 + uVar2) / uVar2;
        uVar2 = *(int*)((char*)pCtx + 0x50) >> 0x1f;
        nRowCount = (int)(((*(int*)((char*)pCtx + 0x50) ^ uVar2) - uVar2) + -1 + iVar3) / iVar3;
    } else {
        dwColCount = *(uint32_t*)((char*)pCtx + 0x78);
        nRowCount = *(int*)((char*)pCtx + 0x7c);
    }
    
    // Step 3: Iterate columns/rows
    uVar2 = *(uint32_t*)((char*)pTileAccessData + 0x20);
    do {
        if (nRowCount <= (int)uVar2) {
            return 0;
        }
        for (uVar5 = *(uint32_t*)((char*)pTileAccessData + 0x1c); (int)uVar5 < (int)dwColCount; uVar5 = uVar5 + 1) {
            // Read tile data for each layer
            iVar3 = *(int*)((char*)pCtx + 0);
            iVar6 = 0;
            if (0 < *(int*)((char*)pTileAccessData + 0)) {
                pLayerIndex = (int*)((char*)pTileAccessData + 0x24);
                do {
                    iVar8 = 0;
                    if (0 < anLayerTileCount[iVar6]) {
                        uVar7 = adLayerStride[iVar6];
                        do {
                            iVar4 = pfnReadTileData(
                                pTileData,
                                adLayerWidth[iVar6],
                                adLayerHeight[iVar6],
                                pLayerIndex,
                                iVar3,
                                uVar7
                            );
                            if (iVar4 < 0) {
                                return -0x15;
                            }
                            iVar3 = iVar3 + 0x80;
                            iVar8 = iVar8 + 1;
                        } while (iVar8 < anLayerTileCount[iVar6]);
                    }
                    iVar6 = iVar6 + 1;
                    pLayerIndex = pLayerIndex + 1;
                } while (iVar6 < *(int*)((char*)pTileAccessData + 0));
            }
            
            // Check tile access boundary
            iVar3 = ROOM_CheckTileAccessBoundary(pTileData, pTileAccessData, pCtx, uVar5, uVar2);
            if (iVar3 != 0) {
                return -0x19;
            }
            
            // Mode-specific handling
            pTileData_00 = (uint16_t*)(*(int*)((char*)pCtx + 0));
            switch(dwMode) {
            case 1:
                if (0 < nTotalTileCount) {
                    pLayerIndex = (int*)nTotalTileCount;
                    do {
                        pfnClearTileData(pTileData_00);
                        pTileData_00 = pTileData_00 + 0x40;
                        pLayerIndex = (int*)((int)pLayerIndex + -1);
                    } while (pLayerIndex != (int*)0x0);
                }
                ROOM_ResolveTileState(pCtx, pTileData, (int*)((char*)pTileAccessData + 0), uVar5, uVar2);
                goto switchD_6001da41_caseD_3;
            case 2:
                if (0 < nTotalTileCount) {
                    pLayerIndex = (int*)nTotalTileCount;
                    do {
                        ANIM_InterpolateCelFrame(pTileData_00);
                        pTileData_00 = pTileData_00 + 0x40;
                        pLayerIndex = (int*)((int)pLayerIndex + -1);
                    } while (pLayerIndex != (int*)0x0);
                }
                uVar7 = 4;
                break;
            case 4:
                iVar3 = nTotalTileCount;
                if (0 < nTotalTileCount) {
                    do {
                        ROOM_InterpolateTileData(pTileData_00);
                        pTileData_00 = pTileData_00 + 0x40;
                        iVar3 = iVar3 + -1;
                    } while (iVar3 != 0);
                }
                uVar7 = 2;
                break;
            case 8:
                iVar3 = nTotalTileCount;
                if (0 < nTotalTileCount) {
                    do {
                        iVar3 = iVar3 + -1;
                        *pTileData_00 = (short)((short)*pTileData_00 + 8 >> 4) + 0x80;
                        pTileData_00 = pTileData_00 + 0x40;
                    } while (iVar3 != 0);
                }
                uVar7 = 1;
                break;
            default:
                goto switchD_6001da41_caseD_3;
            }
            
            ROOM_ApplyTileStateDelta(pCtx, pTileData, (int*)((char*)pTileAccessData + 0), uVar5, uVar2, uVar7);
            
switchD_6001da41_caseD_3:
            if (*(int*)((char*)pCtx + 0x1c) != 0) {
                uVar7 = (int)(uVar5 + 1) % (int)dwColCount;
                *(uint32_t*)((char*)pTileAccessData + 0x1c) = uVar7;
                if (uVar7 == 0) {
                    *(uint32_t*)((char*)pTileAccessData + 0x20) = uVar2 + 1;
                    if (uVar2 + 1 == nRowCount) {
                        *(uint32_t*)((char*)pTileAccessData + 0x20) = 0;
                        return 0;
                    }
                } else {
                    *(uint32_t*)((char*)pTileAccessData + 0x20) = uVar2;
                }
                *(uint32_t*)((char*)pCtx + 0x0c) = uVar5;
                *(uint32_t*)((char*)pCtx + 0x04) = uVar5;
                *(uint32_t*)((char*)pCtx + 0x10) = uVar2;
                *(uint32_t*)((char*)pCtx + 0x08) = uVar2;
                return 1;
            }
        }
        uVar2 = uVar2 + 1;
    } while(true);
}
