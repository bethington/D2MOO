#include "../provider_runtime.h"

// Forward declarations for D2MOO helpers invoked by this function.
// These resolve at link time against the D2MOO translation units.
extern "C" int   __fastcall GetSkillAnimationIndex(void* pTarget);
extern "C" void  __fastcall CreateMissileRing(void* pPlayer, void* pTarget);
extern "C" void  __fastcall SKILLS_ApplyTimedSkillEffect(void* pPlayer, void* pTarget, int nSkillId, int* pnSkillLevel);

// D2MOO_REIMPL_EXPORT: SKILLS_CreateSkillMissile
extern "C" int __fastcall SKILLS_CreateSkillMissile(void* pPlayer, void* pTarget, int nSkillId, int* pnSkillLevel)
{
	// sgptDataTables (a.k.a. g_pDataTables) is a pointer variable; the
	// decompile's bare _DAT_000fe898 refers to its VALUE (the struct base),
	// so deref the resolved address exactly once.
	char* base = (char*)*(void**)D2MOO_Resolve("g_pDataTables");
	if (!base)
		return -1; // resolver missing / name unknown -> obvious mismatch sentinel

	// 1) If pTarget is non-NULL, set flag 0x40 in target's dwFlags field
	//    (UnitAny::dwFlags lives at offset 0x44).
	if (pTarget != (void*)0)
		*(uint32_t*)((char*)pTarget + 0x44) |= 0x40u;

	// 2/4) Validate nSkillId and (later) nAnimIndex against the
	//      sgptDataTables count fields. Load the bounds + skill table base
	//      once; reuse them in the conditional exactly as the decompile does.
	int nSkillCount    = *(int*)(base + 0xba0);
	int nSkillTableBase= *(int*)(base + 0xb98);
	int nAnimCount     = *(int*)(base + 0xb6c);

	if ((-1 < nSkillId) &&
		(nSkillId < nSkillCount) &&
		((nSkillId * 0x23c + nSkillTableBase) != 0))
	{
		// 3) Look up the skill's animation index (caller's helper).
		int nAnimIndex = GetSkillAnimationIndex(pTarget);

		// 4) Validate nAnimIndex against the animation table count.
		if ((-1 < nAnimIndex) && (nAnimIndex < nAnimCount))
		{
			// 5/6) Spawn the missile ring and apply the timed skill effect.
			CreateMissileRing(pPlayer, pTarget);
			SKILLS_ApplyTimedSkillEffect(pPlayer, pTarget, nSkillId, pnSkillLevel);
			return 1;
		}
	}

	// 7) Any failure path (invalid skill or invalid animation index).
	return 0;
}
