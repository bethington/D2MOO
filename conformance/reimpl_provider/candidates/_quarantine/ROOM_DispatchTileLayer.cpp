#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ROOM_DispatchTileLayer
// NEEDS GLOBAL: g_pfnDrawTileClear
// NEEDS GLOBAL: g_pfnPreDrawTile
// NEEDS GLOBAL: g_pfnRenderTileLayer
// NEEDS GLOBAL: g_pfnRenderFlush

extern "C" int __stdcall TILE_EncodeLayer(int nLayerIdx, void* pUnused);

typedef void (*PFN_DrawTileClear)(void* pBuffer);
typedef void (*PFN_PreDrawTile)(void* pBuffer, uint32_t nLayerIdx);
typedef void (*PFN_RenderTileLayer)(void* pUnused, uint32_t nIdx1, uint32_t nIdx2, uint32_t* pnLayerCount, void* pBuffer, void* pTileBuffer);
typedef void (*PFN_RenderFlush)(void);

extern "C" int ROOM_DispatchTileLayer(
    void* pRoom_,           // EBX
    uint32_t dwUnused,  // ESI
    void* pTileCtx_,        // EDI
    void* pTileCoord_,      // EDX
    uint32_t dwTileIndex  // ECX
)
{
    char* pRoom = (char*)pRoom_;
    char* pTileCtx = (char*)pTileCtx_;

    // Resolve function pointer globals (these are pointer variables)
    void* pfnDC = D2MOO_Resolve("g_pfnDrawTileClear");
    void* pfnPD = D2MOO_Resolve("g_pfnPreDrawTile");
    void* pfnRT = D2MOO_Resolve("g_pfnRenderTileLayer");
    void* pfnRF = D2MOO_Resolve("g_pfnRenderFlush");
    if (!pfnDC || !pfnPD || !pfnRT || !pfnRF)
        return -0x7FFFFFFF;

    PFN_DrawTileClear fnDrawTileClear = *(PFN_DrawTileClear*)pfnDC;
    PFN_PreDrawTile fnPreDrawTile = *(PFN_PreDrawTile*)pfnPD;
    PFN_RenderTileLayer fnRenderTileLayer = *(PFN_RenderTileLayer*)pfnRT;
    PFN_RenderFlush fnRenderFlush = *(PFN_RenderFlush*)pfnRF;

    int nField00 = *(int*)(pTileCtx + 0x00);
    int nTotalLayerData = 0;
    int iVar10;  // reused across first loop and inner loops

    uint32_t adwLayerDataSize[4] = {0,0,0,0};
    uint32_t adwLayerIndex1[4]   = {0,0,0,0};
    uint32_t adwLayerIndex2[4]   = {0,0,0,0};
    uint32_t adwLayerIndex3[6]   = {0,0,0,0,0,0};
    uint8_t abTileBuffer[960] = {0};

    if (0 < nField00) {
        uint32_t* pnLayerIndex = (uint32_t*)(pTileCtx + 0x04);
        int nLayerDataOffset = 0;
        char* pbVar9 = pRoom + 0x379c;
        int iVar11 = 0;
        int nField37e0 = *(int*)(pRoom + 0x37e0);
        int* pAdwLayerIndex1 = *(int**)(pTileCtx + 0x34);

        iVar10 = 0;
        while ((int)(pbVar9 - (pRoom + 0x379c)) < nField00) {
            if (nField37e0 == 0) {
                *pnLayerIndex = 0;
            }
            int iVar8 = pAdwLayerIndex1[3] + iVar10;
            iVar10 += 0x18;
            int size = *(int*)(iVar8 + 8) * *(int*)(iVar8 + 4);
            *(uint32_t*)((char*)adwLayerDataSize + iVar11) = (uint32_t)size;
            nTotalLayerData += size;

            uint8_t bVar2 = (uint8_t)*pbVar9;
            *(uint32_t*)((char*)adwLayerIndex2 + iVar11) =
                *(uint32_t*)(pRoom + 0x3704 + (uint32_t)(uint8_t)pbVar9[4] * 4u);
            pbVar9++;
            *(uint32_t*)((char*)adwLayerIndex3 + iVar11) =
                *(uint32_t*)(pRoom + 0x36f4 + (uint32_t)bVar2 * 4u);

            int ptr37cc = *(int*)(pRoom + 0x37cc);
            int* piVar1 = (int*)(ptr37cc + nLayerDataOffset);
            nLayerDataOffset += 0x10;
            *(uint32_t*)((char*)adwLayerIndex1 + iVar11) =
                *(uint32_t*)(pRoom + 0x36e4 + (*piVar1) * 4);

            pnLayerIndex++;
            iVar11 += 4;
        }
    }

    uint32_t nTileIndex = *(uint32_t*)(pTileCtx + 0x20);
    int pRoom_nField7c = *(int*)(pRoom + 0x7c);

    if ((int)nTileIndex < pRoom_nField7c) {
        do {
            uint32_t pTileCoord_00 = *(uint32_t*)(pTileCtx + 0x1c);
            uint32_t pRoom_nField78 = (uint32_t)(*(int*)(pRoom + 0x78));
            if (pTileCoord_00 < pRoom_nField78) {
                do {
                    iVar10 = *(int*)(pRoom + 0x4834);  // nTileBufferBase
                    ROOM_DispatchTileLayer(pRoom, dwUnused, pTileCtx, (void*)(uintptr_t)pTileCoord_00, nTileIndex);

                    if (0 < nTotalLayerData) {
                        int pnLayerBase = nTotalLayerData;
                        char* iVar11_2 = (char*)iVar10;
                        do {
                            fnDrawTileClear(iVar11_2);
                            iVar11_2 += 0x80;
                            pnLayerBase--;
                        } while (pnLayerBase != 0);
                    }

                    int nLayerCount = 0;
                    if (0 < nField00) {
                        uint32_t* pnLayerBase = (uint32_t*)(pTileCtx + 0x04);
                        do {
                            int iVar11_3 = 0;
                            if (0 < (int)adwLayerDataSize[nLayerCount]) {
                                uint32_t uVar6 = adwLayerIndex2[nLayerCount];
                                uint32_t uVar3 = adwLayerIndex1[nLayerCount];
                                uint32_t uVar4 = adwLayerIndex3[nLayerCount];
                                do {
                                    fnPreDrawTile((void*)(uintptr_t)iVar10, uVar3);
                                    fnRenderTileLayer((void*)(uintptr_t)dwUnused, uVar6, uVar4, pnLayerBase, (void*)(uintptr_t)iVar10, abTileBuffer);
                                    iVar10 = (int)((char*)NULL + iVar10 + 0x80); // iVar10 += 0x80
                                    iVar11_3++;
                                } while (iVar11_3 < (int)adwLayerDataSize[nLayerCount]);
                            }
                            nLayerCount++;
                            pnLayerBase++;
                        } while (nLayerCount < nField00);
                    }

                    uint32_t uVar6 = *(uint32_t*)(pTileCtx + 0x18) + 1u;
                    *(uint32_t*)(pTileCtx + 0x18) = uVar6;
                    uint32_t dwField37d0 = *(uint32_t*)(pRoom + 0x37d0);
                    uint64_t uVar5 = (uint64_t)uVar6 % (uint64_t)dwField37d0;

                    if ((int)uVar5 == 0) {
                        uint64_t divq = (uint64_t)uVar6 / (uint64_t)dwField37d0;
                        uint32_t layerIdx = (uint32_t)((divq - 1ULL) & 7ULL);
                        if (layerIdx == 7u) {
                            *(uint32_t*)(pTileCtx + 0x18) = 0u;
                        }
                        int iVar10_enc = TILE_EncodeLayer((int)layerIdx, (void*)(uintptr_t)dwUnused);

                        int iVar11_4 = 0;
                        if (0 < nField00) {
                            uint32_t* puVar7 = (uint32_t*)(pTileCtx + 0x04);
                            do {
                                *puVar7 = 0u;
                                iVar11_4++;
                                puVar7++;
                            } while (iVar11_4 < nField00);
                        }

                        fnRenderFlush();
                        if (iVar10_enc != 0) {
                            return iVar10_enc;
                        }
                    }

                    int nField1c = *(int*)(pRoom + 0x1c);
                    if (nField1c != 0) {
                        uint32_t newCoord = (pTileCoord_00 + 1u) % (uint32_t)(*(int*)(pRoom + 0x78));
                        *(int*)(pRoom + 0x0c) = (int)pTileCoord_00;
                        *(int*)(pRoom + 0x10) = (int)nTileIndex;
                        *(uint32_t*)(pTileCtx + 0x1c) = newCoord;
                        if (newCoord == 0u) {
                            nTileIndex++;
                        }
                        *(uint32_t*)(pTileCtx + 0x20) = nTileIndex;
                        fnRenderFlush();
                        return 1;
                    }

                    pTileCoord_00++;
                } while (pTileCoord_00 < (uint32_t)(*(int*)(pRoom + 0x78)));
            }
            nTileIndex++;
        } while ((int)nTileIndex < *(int*)(pRoom + 0x7c));
    }

    fnRenderFlush();
    return 0;
}
