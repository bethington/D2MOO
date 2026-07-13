#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SKILLS_GetReqSkillTxtRecord
extern "C" void* __fastcall SKILLS_GetReqSkillTxtRecord(int nSkillId)
{
	// Resolve g_pDataTables by name (pointer variable -> deref once).
	char* base = (char*)*(void**)D2MOO_Resolve("g_pDataTables");
	if (base == 0) return (void*)0; // resolver missing or uninitialized

	// 1) Validate nSkillId in [0, nSkillTableCount)  (offset 0xba0)
	if (nSkillId < 0) return (void*)0;
	if (nSkillId >= *(int*)(base + 0xba0)) return (void*)0;

	// 2) pSkillsRow = nSkillId * 0x23c + g_pSkillsTxt  (offset 0xb98)
	char* pSkillsRow = (char*)(nSkillId * 0x23c + *(int*)(base + 0xb98));
	if (pSkillsRow == 0) return (void*)0;

	// 3) wReqSkillDescId (signed short) at SkillsTxt +0x194
	int nReqSkillDescId = (int)(short)*(short*)(pSkillsRow + 0x194);
	if (nReqSkillDescId < 0) return (void*)0;
	if (nReqSkillDescId >= *(int*)(base + 0xb94)) return (void*)0; // nSkillDescCount

	// 4) Return nReqSkillDescId * 0x120 + g_pSkillDescTxt  (offset 0xb8c)
	return (void*)(nReqSkillDescId * 0x120 + *(int*)(base + 0xb8c));
}
