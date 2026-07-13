#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SKILLS_GetSkillDescField0x34
// Disasm: MOV EAX,[ESP+0x4]; TEST EAX,EAX; JL out; MOV ECX,[0x6fde9e1c];
// CMP EAX,[ECX+0xc5c]; JGE out; MOV ECX,[0x6fdf0b38]; IMUL EAX,EAX,0x9c;
// ADD EAX,ECX; MOV EAX,[EAX+0x34]; RET 0x4.
// Both g_pDataTables (DataTables*) and g_dat_6fdf0b38 (g_pLevelDefRows in
// the decompile; SkillDescEntry*) are POINTER VARIABLES -- resolve, then
// deref once to obtain the base struct/array address.
extern "C" int __stdcall SKILLS_GetSkillDescField0x34(int nSkillDescIndex)
{
	// g_pDataTables: pointer-to-DataTables stored at 0x6fde9e1c.
	// nSkillsTblCount sits at struct offset 0xc5c.
	void* _g_pDataTables = D2MOO_Resolve("g_pDataTables");
	if (!_g_pDataTables) return -0x7FFFFFFF; // resolver missing -> obvious mismatch
	char* pDataTables = (char*)*(void**)_g_pDataTables;
	if (!pDataTables) return -0x7FFFFFFF;

	// Bounds check (signed): reject nSkillDescIndex < 0 OR >= nSkillsTblCount.
	if (nSkillDescIndex < 0) return 0;
	int nSkillsTblCount = *(int*)(pDataTables + 0x0c5c);
	if (nSkillDescIndex >= nSkillsTblCount) return 0;

	// g_dat_6fdf0b38 (Ghidra: g_pLevelDefRows): SkillDescEntry* base.
	// Stride per entry = 0x9c, field offset = 0x34.
	void* _g_LevelDefRows = D2MOO_Resolve("g_dat_6fdf0b38");
	if (!_g_LevelDefRows) return -0x7FFFFFFF;
	char* levelDefRows = (char*)*(void**)_g_LevelDefRows;
	if (!levelDefRows) return -0x7FFFFFFF;

	return *(int*)(levelDefRows + nSkillDescIndex * 0x9c + 0x34);
}
