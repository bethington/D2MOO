#include "../provider_runtime.h"

// Forward declarations for subroutines called by SKILLS_CalculateSkillResult
extern "C" void __stdcall SKILLS_AddRandomSkillDelay();
extern "C" void __stdcall GAME_ExecuteSkillWithRandom();
extern "C" int __stdcall Unwind_6fd179c0(void* pUnit, int arg2, int nSkillId, uint32_t dwParam4);
extern "C" void __stdcall ApplyAreaEffectToUnits(int* pUnit, void* arg2, int arg3, int arg4, int arg5, void* arg6);

// D2MOO_REIMPL_EXPORT: SKILLS_CalculateSkillResult
extern "C" int __fastcall SKILLS_CalculateSkillResult(uint32_t dwParam1, void* pUnit, int nSkillId, uint32_t dwParam4)
{
    char* base = (char*)*(void**)D2MOO_Resolve("g_pDataTables");
    if (!base)
        return 0;

    // dwParam1 not consumed in visible body (per plate comment)

    // Validate nSkillId against skill record count
    if (nSkillId < 0)
        return 0;
    if (nSkillId >= *(int*)(base + 0xBA0))
        return 0;

    // Compute skill record pointer (0x23C stride per record)
    int iVar2 = nSkillId * 0x23C + *(int*)(base + 0xB98);
    if (iVar2 == 0)
        return 0;

    // Validate nField48 against DataTables upper bound
    short nField48 = *(short*)(iVar2 + 0x48);
    if (nField48 <= 0)
        return 0;
    if ((int)nField48 >= *(int*)(base + 0xB6C))
        return 0;

    // Set unit flag bit 0x40 at UnitAny+0xC4
    if (pUnit != (void*)0) {
        *(uint32_t*)((char*)pUnit + 0xC4) |= 0x40u;
    }

    // Local scratch region (0x1C dwords = 0x70 bytes) - locals don't escape
    volatile uint32_t local_buf[0x1C];
    for (int i = 0; i < 0x1C; i++)
        local_buf[i] = 0;
    // Dead merge ops into locals (consumed nowhere, but match decompile)
    // wSkillField12E |= *(ushort*)(iVar2 + 0x12E);
    // dwSkillField130 |= 1u | *(uint*)(iVar2 + 0x130);
    // if (*(uint*)(iVar2 + 0x134) != 0) dwSkillField134 = *(uint*)(iVar2 + 0x134);

    SKILLS_AddRandomSkillDelay();
    GAME_ExecuteSkillWithRandom();
    int unwindResult = Unwind_6fd179c0(pUnit, *(int*)(iVar2 + 100), nSkillId, dwParam4);

    // Apply area effect to units (only if pUnit is non-null)
    if (pUnit != (void*)0) {
        ApplyAreaEffectToUnits((int*)pUnit, (void*)0, 0, unwindResult, 0x8583, 0x6FCC1210);
    }

    return 1;
}
