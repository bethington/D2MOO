#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ROOM_AllocateTileStateFromPool
// NEEDS GLOBAL: g_pTileStateBase
// NEEDS GLOBAL: g_dwTileStateCount
// NEEDS GLOBAL: g_dwTileStateOffset
// NEEDS GLOBAL: g_pTileStateCurr
// NEEDS GLOBAL: g_dwTileStateBit

extern "C" void* ROOM_AllocTileStateEntry();
extern "C" int ROOM_AllocTileStateNodeList(void* ctx);

extern "C" void* __stdcall ROOM_AllocateTileStateFromPool(uint32_t dwSize)
{
    // Resolve pointer variables (g_p*) - deref once
    void* res_pTileStateBase = D2MOO_Resolve("g_pTileStateBase");
    if (!res_pTileStateBase) return (void*)0xDEADBEEF;
    char* g_pTileStateBase = (char*)*(void**)res_pTileStateBase;

    void* res_pTileStateCurr = D2MOO_Resolve("g_pTileStateCurr");
    if (!res_pTileStateCurr) return (void*)0xDEADBEEF;
    uint32_t g_pTileStateCurr = *(uint32_t*)res_pTileStateCurr;

    // Resolve data variables (g_dw*)
    void* res_dwTileStateCount = D2MOO_Resolve("g_dwTileStateCount");
    if (!res_dwTileStateCount) return (void*)0xDEADBEEF;
    uint32_t g_dwTileStateCount = *(uint32_t*)res_dwTileStateCount;

    void* res_dwTileStateOffset = D2MOO_Resolve("g_dwTileStateOffset");
    if (!res_dwTileStateOffset) return (void*)0xDEADBEEF;
    uint32_t g_dwTileStateOffset = *(uint32_t*)res_dwTileStateOffset;

    void* res_dwTileStateBit = D2MOO_Resolve("g_dwTileStateBit");
    if (!res_dwTileStateBit) return (void*)0xDEADBEEF;
    uint32_t g_dwTileStateBit = *(uint32_t*)res_dwTileStateBit;

    // Local variables
    uint32_t* puVar1;
    uint32_t* puVar2;
    char* pcVar3;
    char cVar4;
    uint8_t bVar5;
    uint32_t uVar6;
    int iVar7;
    uint32_t* puVar8;
    uint32_t uVar9;
    int iVar10;
    int* piVar11;
    void* pvVar12;
    int iVar13;
    uint32_t dwBitMask;
    uint32_t dwBitMask2;
    uint32_t dwNodeIndex;

    // Algorithm - translate decompiled code literally
    puVar2 = (uint32_t*)((int)g_pTileStateBase + (int)(g_dwTileStateCount * 0x14));
    uVar6 = dwSize + 0x17 & 0xfffffff0;
    iVar7 = ((int)(dwSize + 0x17) >> 4) + -1;
    bVar5 = (uint8_t)iVar7;
    dwSize = g_dwTileStateOffset;

    if (iVar7 < 0x20) {
        dwBitMask = 0xffffffff >> (bVar5 & 0x1f);
        dwBitMask2 = 0xffffffff;
    } else {
        dwBitMask2 = 0xffffffff >> (bVar5 - 0x20 & 0x1f);
        dwBitMask = 0;
    }

    for (; (dwSize < (uint32_t)puVar2 &&
            ((*(uint32_t*)(dwSize + 4) & dwBitMask2) == 0 && (*(uint32_t*)dwSize & dwBitMask) == 0));
         dwSize = (uint32_t)(dwSize + 0x14)) {
    }

    puVar8 = (uint32_t*)g_pTileStateBase;
    if ((uint32_t*)dwSize == puVar2) {
        for (; ((uint32_t*)puVar8 < (uint32_t*)g_dwTileStateOffset &&
                ((puVar8[1] & dwBitMask2) == 0 && (*puVar8 & dwBitMask) == 0)); puVar8 = puVar8 + 5) {
        }
        dwSize = (uint32_t)puVar8;
        if (puVar8 == (uint32_t*)g_dwTileStateOffset) {
            for (; ((uint32_t*)puVar8 < puVar2 && (puVar8[2] == 0)); puVar8 = puVar8 + 5) {
            }
            pvVar12 = (void*)g_pTileStateBase;
            dwSize = (uint32_t)puVar8;
            if (puVar8 == puVar2) {
                for (; ((uint32_t*)pvVar12 < (uint32_t*)g_dwTileStateOffset && (*(int*)((int)pvVar12 + 8) == 0));
                     pvVar12 = (void*)((int)pvVar12 + 0x14)) {
                }
                dwSize = (uint32_t)pvVar12;
                if ((pvVar12 == (void*)g_dwTileStateOffset) &&
                    (dwSize = (uint32_t)ROOM_AllocTileStateEntry(), (void*)dwSize == (void*)0x0)) {
                    return (void*)0;
                }
            }
            iVar7 = ROOM_AllocTileStateNodeList((void*)dwSize);
            **(int**)(dwSize + 0x10) = iVar7;
            if (**(int**)(dwSize + 0x10) == -1) {
                return (void*)0;
            }
        }
    }

    puVar2 = *(uint32_t**)(dwSize + 0x10);
    dwNodeIndex = *puVar2;
    if ((dwNodeIndex == 0xffffffff) ||
        ((puVar2[dwNodeIndex + 0x31] & dwBitMask2) == 0 &&
         (puVar2[dwNodeIndex + 0x11] & dwBitMask) == 0)) {
        dwNodeIndex = 0;
        puVar8 = puVar2 + 0x11;
        if ((puVar2[0x31] & dwBitMask2) == 0 && (puVar2[0x11] & dwBitMask) == 0) {
            do {
                puVar1 = puVar8 + 0x21;
                dwNodeIndex = dwNodeIndex + 1;
                puVar8 = puVar8 + 1;
            } while ((*puVar1 & dwBitMask2) == 0 && (dwBitMask & *puVar8) == 0);
        }
    }

    iVar7 = 0;
    puVar8 = puVar2 + dwNodeIndex * 0x81 + 0x51;
    uVar9 = puVar2[dwNodeIndex + 0x11] & dwBitMask;
    if (uVar9 == 0) {
        uVar9 = puVar2[dwNodeIndex + 0x31] & dwBitMask2;
        iVar7 = 0x20;
    }
    for (; -1 < (int)uVar9; uVar9 = uVar9 << 1) {
        iVar7 = iVar7 + 1;
    }

    piVar11 = (int*)puVar8[iVar7 * 2 + 1];
    iVar10 = *piVar11 - uVar6;
    iVar13 = (iVar10 >> 4) + -1;
    if (0x3f < iVar13) {
        iVar13 = 0x3f;
    }

    *(uint32_t*)res_dwTileStateOffset = dwSize;

    if (iVar13 != iVar7) {
        if (piVar11[1] == piVar11[2]) {
            if (iVar7 < 0x20) {
                pcVar3 = (char*)((int)puVar2 + iVar7 + 4);
                uVar9 = ~(0x80000000U >> ((uint8_t)iVar7 & 0x1f));
                puVar2[dwNodeIndex + 0x11] = uVar9 & puVar2[dwNodeIndex + 0x11];
                *pcVar3 = *pcVar3 + -1;
                if (*pcVar3 == '\0') {
                    *(uint32_t*)dwSize = *(uint32_t*)dwSize & uVar9;
                }
            } else {
                pcVar3 = (char*)((int)puVar2 + iVar7 + 4);
                uVar9 = ~(0x80000000U >> ((uint8_t)iVar7 - 0x20 & 0x1f));
                puVar2[dwNodeIndex + 0x31] = puVar2[dwNodeIndex + 0x31] & uVar9;
                *pcVar3 = *pcVar3 + -1;
                if (*pcVar3 == '\0') {
                    *(uint32_t*)(dwSize + 4) = *(uint32_t*)(dwSize + 4) & uVar9;
                }
            }
        }
        *(int*)(piVar11[2] + 4) = piVar11[1];
        *(int*)(piVar11[1] + 8) = piVar11[2];
        if (iVar10 == 0) goto done_finalize_return;
        piVar11[1] = puVar8[iVar13 * 2 + 1];
        piVar11[2] = (int)(puVar8 + iVar13 * 2);
        (puVar8 + iVar13 * 2)[1] = (uint32_t)piVar11;
        *(int**)(piVar11[1] + 8) = piVar11;
        if (piVar11[1] == piVar11[2]) {
            cVar4 = *(char*)(iVar13 + 4 + (int)puVar2);
            bVar5 = (uint8_t)iVar13;
            if (iVar13 < 0x20) {
                *(char*)(iVar13 + 4 + (int)puVar2) = cVar4 + '\x01';
                if (cVar4 == '\0') {
                    *(uint32_t*)dwSize = *(uint32_t*)dwSize | 0x80000000U >> (bVar5 & 0x1f);
                }
                puVar2[dwNodeIndex + 0x11] = puVar2[dwNodeIndex + 0x11] | 0x80000000U >> (bVar5 & 0x1f);
            } else {
                *(char*)(iVar13 + 4 + (int)puVar2) = cVar4 + '\x01';
                if (cVar4 == '\0') {
                    *(uint32_t*)(dwSize + 4) = *(uint32_t*)(dwSize + 4) | 0x80000000U >> (bVar5 - 0x20 & 0x1f);
                }
                puVar2[dwNodeIndex + 0x31] = puVar2[dwNodeIndex + 0x31] | 0x80000000U >> (bVar5 - 0x20 & 0x1f);
            }
        }
    }
    if (iVar10 != 0) {
        *piVar11 = iVar10;
        *(int*)(iVar10 + -4 + (int)piVar11) = iVar10;
    }
done_finalize_return:
    piVar11 = (int*)((int)piVar11 + iVar10);
    *piVar11 = uVar6 + 1;
    *(uint32_t*)((int)piVar11 + (uVar6 - 4)) = uVar6 + 1;
    uVar6 = *puVar8;
    *puVar8 = uVar6 + 1;
    if (((uVar6 == 0) && ((void*)dwSize == (void*)g_pTileStateCurr)) &&
        (dwNodeIndex == g_dwTileStateBit)) {
        *(uint32_t*)res_pTileStateCurr = 0;
    }
    *puVar2 = dwNodeIndex;
    return (void*)(piVar11 + 1);
}
