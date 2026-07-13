#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SKILLS_GetSkillRange
// EAX=skillId -> EAX range (or 20 default)
extern "C" int __fastcall SKILLS_GetSkillRange(int skillId)
{
	// Resolve g_pDataTables by verified name (address of the pointer variable).
	void* _g = D2MOO_Resolve("g_pDataTables");
	if (_g == nullptr)
		return -1; // resolver missing -> obvious mismatch sentinel

	// g_p* is a POINTER variable -> deref once to get DataTables* base.
	char* base = (char*)*(void**)_g;
	if (base == nullptr)
		return -1; // DataTables* null -> obvious mismatch sentinel

	// Bounds check: skillId < 0 OR skillId >= nSkillsCount (at +0xBA0)
	int nSkillsCount = *(int*)(base + 0xBA0);
	if (skillId < 0 || nSkillsCount <= skillId)
		return 0x14; // 20

	// pSkillRowsAlt lives at +0xB98 (4 bytes before nSkillsCount).
	char* pSkillRowsAlt = (char*)*(void**)(base + 0xB98);

	// MOVSX EAX, word ptr [ECX + EAX*1 + 0x12C]  (sign-extends short)
	int range = (int)*(short*)(pSkillRowsAlt + skillId * 0x23C + 0x12C);

	// TEST EAX,EAX / JG: only positive ranges pass through.
	if (range < 1)
		return 0x14; // 20

	return range;
}
