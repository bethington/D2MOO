#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SKILLS_GetBaseMinDamage
extern "C" int __fastcall SKILLS_GetBaseMinDamage(int nSkillId)
{
	// g_pDataTables is a POINTER VARIABLE -> deref the resolved address ONCE
	// to obtain the DataTablesContext struct base used in every offset below.
	void* _g_pDataTables = D2MOO_Resolve("g_pDataTables");
	if (_g_pDataTables == nullptr)
		return -0x7FFFFFFF; // resolver not injected -> obvious mismatch sentinel

	char* base = (char*)*(void**)_g_pDataTables;
	if (base == nullptr)
		return -0x7FFFFFFF; // null context -> obvious mismatch sentinel

	// Range check: (-1 < nSkillId) && (nSkillId < nField0BA0)
	int nField0BA0 = *(int*)(base + 0xBA0);
	if (nSkillId < 0 || nField0BA0 <= nSkillId)
		return 0;

	// iVar1 = nSkillId * 0x23C + (int)g_pDataTables->pSkillRowsAlt
	int pSkillRowsAlt = *(int*)(base + 0xB98);
	int iVar1 = nSkillId * 0x23c + pSkillRowsAlt;
	if (iVar1 == 0)
		return 0;

	return *(int*)(iVar1 + 0x148);
}
