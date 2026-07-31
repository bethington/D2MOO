// D2MOO_REIMPL_EXPORT: DATATBLS_GetSkillDescSlotEntry
//
// Reimplementation of DATATBLS_GetSkillDescSlotEntry (D2Common.dll @ 0x6fdd4b40).
// Reads GLOBAL game state via the injected D2MOO_Resolve resolver -- NEVER a
// hardcoded address, NEVER an extern. Cannot be proven statically.
//
// Algorithm (per decompile):
//   1. dwSlotIndex < 16                                         (early-exit)
//   2. nSkillId in [0, nMonStatsTxtRecordCount)
//   3. dwDescTableIndex = (short) pMonStatsTxt[nSkillId].wMonStatsEx
//   4. dwDescTableIndex in [0, nMonStats2TxtRecordCount)
//   5. dwDescEntryOffset = dwDescTableIndex * 0x134 + pMonStats2Txt, != 0
//   6. dwSubIndex < *(byte*)(dwDescEntryOffset + 0x15 + dwSlotIndex)
//   7. dwSlotEntryIndex = *(byte*)(dwDescEntryOffset + dwSlotIndex*0xC + 0x26 + dwSubIndex)
//   8. dwSlotEntryIndex < g_nSkillCalcCount
//   9. return *(uint*) g_pMonPlacePool[dwSlotEntryIndex].aData
//
// Calling convention (per AUTHORITATIVE ABI: RET 0xc => 3 stack dwords):
//   __stdcall, all 3 params on the stack. (The ABI also notes ESI is incoming,
//   but the C signature lists exactly three params and RET 0xc == 12 bytes ==
//   3 stack dwords, so we treat all three as stack -- the prover marshals.)

#include "../provider_runtime.h"

// The following symbols are required by the decompile's algorithm but are not
// present in the resolver name list. The function is therefore skipped until
// these names are added:
// NEEDS GLOBAL: g_nMonStatsTxtRecordCount
// NEEDS GLOBAL: g_nMonStats2TxtRecordCount
// NEEDS GLOBAL: g_nSkillCalcCount
// NEEDS GLOBAL: g_pMonPlacePool

extern "C" uint32_t __stdcall DATATBLS_GetSkillDescSlotEntry(
    int nSkillId, uint32_t dwSlotIndex, uint32_t dwSubIndex)
{
    // g_pDataTables is a POINTER VARIABLE (DataTables**). Per STEP 1 of the
    // mechanical rule, dereference the resolved address ONCE to get the
    // pointer's VALUE (the struct base -- the plate comment calls this
    // sgptDataTables).
    char* sgptDataTables = (char*)*(void**)D2MOO_Resolve("g_pDataTables");
    if (!sgptDataTables) return 0u;

    uint32_t dwDescTableIndex;
    uint32_t dwDescEntryOffset;
    uint32_t dwSlotEntryIndex;

    if (0xFu < dwSlotIndex) return 0u;

    // The decompile then validates nSkillId against g_pDataTables[0]->nMonStatsTxtRecordCount,
    // reads wMonStatsEx from g_pDataTables[0]->pMonStatsTxt[nSkillId], validates the desc-table
    // index against g_pDataTables[0]->nMonStats2TxtRecordCount, indexes into
    // g_pDataTables[0]->pMonStats2Txt (at known offset 0xA90), and ultimately reads the result
    // from g_pMonPlacePool[dwSlotEntryIndex].aData. None of those field/count/pool names
    // are in the resolver list, so any read past this point would be unfounded. Return
    // the obvious wrong-value sentinel (0u) so a misconfig fails loudly.
    (void)nSkillId;
    (void)dwSubIndex;
    (void)dwDescTableIndex;
    (void)dwDescEntryOffset;
    (void)dwSlotEntryIndex;
    return 0u;
}
