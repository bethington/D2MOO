#include "../provider_runtime.h"

extern "C" void __stdcall SKILLS_PropagateRelatedSkillEffects(int, int);
extern "C" void __cdecl Unwind_6fd179c0(...);
extern "C" void* __fastcall CreateSkillPath(void*);
extern "C" void __stdcall SKILLS_ProcessDruidSkills(void*, void*, int, int);
extern "C" void __stdcall SKILLS_ApplyDruidSkillEffects(void*, void*, void*, int);
extern "C" void __stdcall RemoveSkillResourceBuffer(int, int);
extern "C" void __stdcall SKILLS_CreateSkillResource(int, int, void*, uint8_t, uint32_t, uint32_t);

// D2MOO_REIMPL_EXPORT: SKILLS_CreateDruidSkillPathEffect
extern "C" void* __fastcall SKILLS_CreateDruidSkillPathEffect(
    uint32_t dwSkillFlags,
    void* nUnitHandle,
    int pSkillStatList,
    uint32_t dwPosition)
{
    char* base = (char*)*(void**)D2MOO_Resolve("g_pDataTables");
    if (!base) return (void*)0xDEADBEEF;

    if (pSkillStatList < 0) return (void*)0;
    if (*(int*)(base + 0xba0) <= pSkillStatList) return (void*)0;

    void* pDruidSkillTblEntry = (void*)(pSkillStatList * 0x23c + *(int*)(base + 0xb98));
    if (pDruidSkillTblEntry == (void*)0) return (void*)0;

    int nSkillId = *(int*)((char*)pDruidSkillTblEntry + 0);
    if (nSkillId < 0) return (void*)0;
    if (*(int*)(base + 0xc4) <= nSkillId) return (void*)0;

    if (nUnitHandle != (void*)0) {
        *(int*)((char*)nUnitHandle + 0xC4) |= 0x40;
    }

    SKILLS_PropagateRelatedSkillEffects(nSkillId, 1);

    void* pField60 = *(void**)((char*)pDruidSkillTblEntry + 0x60);
    Unwind_6fd179c0(nUnitHandle, pField60, (void*)pSkillStatList, (void*)dwPosition);

    void* pSkillData = CreateSkillPath((void*)0x6FC622B0);
    if (pSkillData == (void*)0) return (void*)0;

    SKILLS_ProcessDruidSkills(nUnitHandle, pSkillData, pSkillStatList, 0);
    SKILLS_ApplyDruidSkillEffects(nUnitHandle, pSkillData, (void*)pSkillStatList, 0);

    void* pEventCtx = pSkillData;
    void* pSVar2 = (void*)pSkillStatList;

    Unwind_6fd179c0();
    Unwind_6fd179c0();

    uint16_t* puVar1 = (uint16_t*)((char*)pDruidSkillTblEntry + 0x84);

    if ((short)*puVar1 >= 0) {
        RemoveSkillResourceBuffer(1, nSkillId);
        int dwDruidSkillTableRecordCount = 0;
        pSkillData = pSVar2;
        uint32_t dwUnused = 0;
        int pUnused;

        do {
            if ((short)*puVar1 < 0) break;
            SKILLS_CreateSkillResource(
                nSkillId, 1, pEventCtx,
                (uint8_t)*puVar1, (uint32_t)pSkillStatList, dwUnused);
            dwDruidSkillTableRecordCount = dwDruidSkillTableRecordCount + 1;
            puVar1 = puVar1 + 1;
            pSkillStatList = (int)pUnused;
        } while ((int)dwDruidSkillTableRecordCount < 3);
    }

    Unwind_6fd179c0();
    return pSkillData;
}
