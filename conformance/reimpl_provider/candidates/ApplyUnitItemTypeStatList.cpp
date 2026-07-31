#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ApplyUnitItemTypeStatList
extern "C" int __stdcall ApplyUnitItemTypeStatList(int nSkillId)
{
	void* resolved = D2MOO_Resolve("g_pDataTables");
	if (!resolved)
		return -0x7FFFFFFF; // resolver missing -> obvious mismatch

	// g_pDataTables[0] is the first DataTables* (value of the pointer variable).
	char* base = (char*)*(void**)resolved;
	if (!base)
		return -0x7FFFFFFF;

	// Bounds check: nSkillId must be in [0, nSkillsTxtRecordCount)
	int nCount = *(int*)(base + 0xBA0);
	if (nSkillId < 0 || nSkillId >= nCount)
		return 0;

	// pSkillsTxt pointer at offset 0xB98
	char* pSkillsTxt = *(char**)(base + 0xB98);
	if (!pSkillsTxt)
		return 0;

	// Read short at record + 0x94 (stride 0x23C)
	short sVar1 = *(short*)(pSkillsTxt + nSkillId * 0x23C + 0x94);

	if (sVar1 > 0)
		return (int)sVar1;
	return 0;
}
