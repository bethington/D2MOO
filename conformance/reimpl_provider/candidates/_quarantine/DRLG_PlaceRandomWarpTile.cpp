#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DRLG_PlaceRandomWarpTile
//
// ABI: register_explicit. pDrlgCtx in EDI, nWarpIndex in EBX.
//   callee-cleans 0 bytes (RET 0). Returns uint in EAX.
//
// The function reads/mutates DRLG context state (LCG RNG, DrlgPreset
// arrays). All global game state is reached through the register-
// supplied context pointer; no symbols are dereferenced by name, so
// D2MOO_Resolve is not needed in this reimpl.

extern "C" uint32_t __stdcall DRLG_PlaceRandomWarpTile(
    void* pDrlgCtx_,    // EDI
    int nWarpIndex_)    // EBX
{
    uint8_t* ctx = (uint8_t*)pDrlgCtx_;
    int nWarpIndex = nWarpIndex_;
    if (!ctx) return 0;

    // Layout offsets within the live D2Common.dll 1.13c binary
    // (Ghidra-inferred; identical offsets are used by the live
    //  original because the harness reads both code paths from the
    //  same module image):
    //   DRLG_Context.pPresetArray   @ ctx  + 0xC0
    //   DRLG_Context.dwRngStateLo   @ ctx  + 0x14
    //   DRLG_Context.dwRngStateHi   @ ctx  + 0x18
    //   DrlgPreset.pTilePositions   @ pr   + 0x00
    //   DrlgPreset.pParent          @ pr   + 0x04
    //   DrlgPreset.pObjectListHead  @ pr   + 0x08
    //   DrlgPreset.dwTileVariant    @ pr   + 0x0C
    //   DrlgPreset.nTileCount       @ pr   + 0x14  (referenced by
    //                                            the decompile's
    //                                            (int)&pTilePositions+2)

    uint8_t** ppDVar1 = *(uint8_t***)(ctx + 0xC0);
    if (!ppDVar1) return 0;

    uint32_t dwCollisionFlags = (*(uint32_t*)ppDVar1) & 4u;

    // pDVar2 = ppDVar1[0x18]   (loaded for the count computation;
    //                          its raw value is not otherwise used).
    (void)ppDVar1[0x18];

    // puVar6 = (undefined1*)((int)&pDVar2[-1].pTilePositions + 2)
    // The Ghidra reconstructed pointer arithmetic + the int cast land
    // on the nTileCount slot inside DrlgPreset[0x17]. Read it as an int.
    int puVar6 = *(int*)(ppDVar1[0x17] + 0x14);
    int nTiles  = puVar6;

    uint32_t dwStartOffset;
    uint32_t dwResult;

    if (puVar6 < 1) {
        dwStartOffset = 0;
        dwResult      = dwCollisionFlags;
    } else {
        // lVar5 = (ulonglong)lo * 0x6AC690C5 + hi
        uint32_t    lo    = *(uint32_t*)(ctx + 0x14);
        uint32_t    hi    = *(uint32_t*)(ctx + 0x18);
        unsigned __int64 lVar5 = (unsigned __int64)lo * 0x6AC690C5ull + hi;
        *(uint32_t*)(ctx + 0x14) = (uint32_t)lVar5;
        *(uint32_t*)(ctx + 0x18) = (uint32_t)(lVar5 >> 32);

        uint32_t nU = (uint32_t)puVar6;
        // Power-of-2 fast path (decompile's reconstructed mask ended up
        // structurally invalid; the canonical check is n & (n-1) == 0).
        if ((nU & (nU - 1u)) == 0u) {
            dwResult      = (uint32_t)lVar5;
            dwStartOffset = (nU - 1u) & (uint32_t)lVar5;
        } else {
            uint32_t dwRange = (uint32_t)lVar5;
            dwResult      = dwRange / nU;
            dwStartOffset = dwRange % nU;
        }
    }

    uint32_t dwIterCount = 0;
    if (puVar6 < 1) return dwResult;

    do {
        // iVar7 = (dwStartOffset + dwIterCount) % puVar6
        int iVar7 = (int)((dwStartOffset + dwIterCount) % (uint32_t)puVar6);

        // compiler artifact: ppDVar3 reload
        uint8_t** ppDVar3 = *(uint8_t***)(ctx + 0xC0);
        if (!ppDVar3) return dwIterCount;

        // tileVariantIndex = (&ppDVar3[0xc]->dwTileVariant)[iVar7]
        int* tileVarBase    = (int*)(ppDVar3[0xC] + 0x0C);
        int  tileVariantIdx = tileVarBase[iVar7 + nWarpIndex];
        // combined index for downstream accesses (tileVariantIdx + nWarpIndex)
        int combinedIdx = tileVariantIdx + nWarpIndex;

        char* dataPtrB = (char*)ppDVar3[0xB];
        int*  piVar4;

        if (dwCollisionFlags == 0) {
            uint32_t pre = *(uint32_t*)(dataPtrB + combinedIdx * 4 - 4);
            if ((pre & 0x1B81u) != 0u) {
                goto next_iter;
            }
            // piVar4 = ppDVar3[0xb]->pParent[combinedIdx]
            int* pParentBase = (int*)(ppDVar3[0xB] + 0x04);
            piVar4 = &pParentBase[combinedIdx];          // alias, see below
            piVar4 = (int*)(ppDVar3[0xB] + 0x04 + combinedIdx * 4);
        } else {
            // piVar4 = *(int**)(dataPtrB + combinedIdx * 4 - 4)
            piVar4 = *(int**)(dataPtrB + combinedIdx * 4 - 4);
        }

        // validate_tile_placement:
        {
            char byte1 = ((char*)ppDVar3[0xB])[combinedIdx * 4 + 2 + 0x08]; // pObjectListHead
            char byte2 = ((char*)ppDVar3[0xB])[combinedIdx * 4 + 2 + 0x0C]; // dwTileVariant

            if ((((uint32_t)piVar4 & 0x1B81u) == 0u)
                && ((byte1 & 0xF) == 3)
                && ((byte2 & 0xF) == 3))
            {
                // Match: invoke PlacePresetLevelObjectAtTile twice.
                // (first call's return value is discarded by original;
                //  second call's return is the function's overall result.)
                // In the harness, the call is patched to the live game's
                // export; this reimpl delegates to a provider-side stub
                // symbol so the proof framework can match the call shape.
                extern uint32_t __stdcall PlacePresetLevelObjectAtTile(
                    uint32_t, int, int, uint32_t, uint32_t, uint32_t);
                PlacePresetLevelObjectAtTile(
                    (uint32_t)(uintptr_t)ctx, nWarpIndex,
                    iVar7 + 1, 0x1Cu, 1u, 0u);
                uint32_t extraout_EAX = PlacePresetLevelObjectAtTile(
                    (uint32_t)(uintptr_t)ctx, nWarpIndex + 1,
                    iVar7 + 1, 0x1Cu,
                    (dwCollisionFlags != 0u) + 2u, 0u);
                return extraout_EAX;
            }
        }

        next_iter:
        dwIterCount = dwIterCount + 1u;
        if (puVar6 <= (int)dwIterCount) return dwIterCount;
    } while (1);
}
