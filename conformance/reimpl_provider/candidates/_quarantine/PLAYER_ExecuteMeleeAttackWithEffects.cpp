#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: PLAYER_ExecuteMeleeAttackWithEffects
extern "C" int __stdcall PLAYER_ExecuteMeleeAttackWithEffects(void* pGameCtx, void* pAttacker)
{
	extern int __stdcall MONSTER_ValidateUnitItemReference(int);
	extern void* __stdcall Unwind_6fd179c0(void*);
	extern int __stdcall ProcessUnitCombatInteraction(void*, void*, void*);
	extern int __stdcall UpdatePlayerWeaponSkills(void*, void*);
	extern int __stdcall ITEMS_ProcessEquippedItemDurability(void);
	extern int __stdcall CreateDruidSkillEffect(void*, int);
	extern int __stdcall ApplyAreaEffectToUnits(void*, void*, int, int, int, void*);
	extern int __stdcall SKILLS_ProcessAttackSequence(int, void*, int);
	extern int __stdcall UNIT_RemoveUnitResourceLink(int, void*);

	int nDruidSkillParam = 0;

	MONSTER_ValidateUnitItemReference((int)pGameCtx);
	void* pTargetUnit = Unwind_6fd179c0((void*)0);

	if ((pTargetUnit == pAttacker) || (pTargetUnit == (void*)0)) {
		return 0;
	}

	int dwAnimFlags = ((int)*(int*)((char*)pAttacker + 0x10)) >> 8 & (int)0x80000001;
	int fHasValidFrame = (dwAnimFlags == 0);
	if (dwAnimFlags < 0) {
		fHasValidFrame = ((dwAnimFlags - 1) | (int)0xFFFFFFFE) == (int)0xFFFFFFFF;
	}

	if (!fHasValidFrame) {
		*(int*)((char*)pAttacker + 0xC4) = *(int*)((char*)pAttacker + 0xC4) | 0x40;
		ProcessUnitCombatInteraction(pGameCtx, pAttacker, pTargetUnit);
		UpdatePlayerWeaponSkills((void*)0, pGameCtx);
		ITEMS_ProcessEquippedItemDurability();
		CreateDruidSkillEffect(pGameCtx, nDruidSkillParam);
		int iVar1 = (int)Unwind_6fd179c0(pAttacker);
		ApplyAreaEffectToUnits(pAttacker, (void*)0, 0, iVar1 + 4, 0x2A783, (void*)0x6FCBED70);
		iVar1 = SKILLS_ProcessAttackSequence((int)pGameCtx, pAttacker, 0);
		return iVar1;
	}

	ProcessUnitCombatInteraction(pGameCtx, pAttacker, pTargetUnit);
	UNIT_RemoveUnitResourceLink((int)pGameCtx, pTargetUnit);
	UpdatePlayerWeaponSkills((void*)0, pGameCtx);
	ITEMS_ProcessEquippedItemDurability();
	CreateDruidSkillEffect(pGameCtx, nDruidSkillParam);
	int nFrameData = SKILLS_ProcessAttackSequence((int)pGameCtx, pAttacker, (int)pTargetUnit);
	return nFrameData;
}
