#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetSkillTreeStateById
extern "C" uint32_t __fastcall GetSkillTreeStateById(int nSkillId, int nSkillIdSearch)
{
	(void)nSkillId; // ECX: unused per spec

	char* tblBase = (char*)D2MOO_Resolve("g_anQuestLookupTbl");
	char* tipBase = (char*)D2MOO_Resolve("g_adTooltipSkillIdField");
	if (!tblBase || !tipBase)
		return 0xFFFFFFFFu; // resolver missing -> obvious wrong-value sentinel

	uint32_t dwIndex = 0;
	int* pnQuestLookupEntry = (int*)tblBase;
	do {
		if (*pnQuestLookupEntry == nSkillIdSearch) {
			return *(uint32_t*)(tipBase + dwIndex * 0x10);
		}
		dwIndex = dwIndex + 1;
		pnQuestLookupEntry = pnQuestLookupEntry + 4;
	} while (dwIndex < 0x29);
	return 0x2a;
}
