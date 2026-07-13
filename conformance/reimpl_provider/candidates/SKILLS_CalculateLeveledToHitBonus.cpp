// skills_calculate_leveled_to_hit_bonus.cpp -- reimpl provider for SKILLS_CalculateLeveledToHitBonus
//
// Original (D2Common @ 0x6fd51730): register_explicit ABI (EAX=nSkillId, EDX=dwSkillLevel,
// ESI preserved), RET 0x0 (0 stack params). Reads global DataTables:
//   g_pDataTables (pointer variable) -> deref once -> struct base
//     +0xB98: pSkillRowsAlt (skill row array base)
//     +0xBA0: nLevelTileYEntryCount (max valid skill index, exclusive)
//   each row is 0x23C bytes:
//     +0x160: nToHitBonusBase (int)
//     +0x164: nToHitBonusPerLevel (int)
// Algorithm: if 0<=nSkillId<count and row!=NULL and (int)dwSkillLevel>0:
//              return (dwSkillLevel-1)*nToHitBonusPerLevel + nToHitBonusBase
//           else: return 0
// The disassembly uses IMUL (signed multiply) and TEST+JL/JG (signed compares).

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SKILLS_CalculateLeveledToHitBonus
extern "C" int __fastcall SKILLS_CalculateLeveledToHitBonus(
	int nSkillId,            // EAX (signed int)
	uint32_t dwSkillLevel, // EDX (unsigned int, signed-compared)
	uint32_t /*unused_ESI*/) // ESI: callee-preserved scratch; not an actual input
{
	// Resolve g_pDataTables by verified name. It is a pointer variable (g_p*),
	// so dereference once to get the struct base.
	void* g_pDataTables_addr = D2MOO_Resolve("g_pDataTables");
	if (g_pDataTables_addr == nullptr)
		return 0; // resolver not injected -> obvious mismatch sentinel

	char* base = (char*)*(void**)g_pDataTables_addr;
	if (base == nullptr)
		return 0; // misconfig sentinel

	// 1. Bounds check: -1 < nSkillId  (disasm: TEST EAX,EAX ; JL exit)
	if (nSkillId < 0)
		return 0;

	// 2. Bounds check: nSkillId < nLevelTileYEntryCount  (disasm: CMP EAX,[ECX+0xba0] ; JGE exit)
	int nLevelTileYEntryCount = *(int*)(base + 0xBA0);
	if (nSkillId >= nLevelTileYEntryCount)
		return 0;

	// 3. Compute row pointer: pSkillRowsAlt + nSkillId*0x23C  (disasm: IMUL + ADD)
	char* pSkillRowsAlt = *(char**)(base + 0xB98);
	char* pRow = pSkillRowsAlt + nSkillId * 0x23C;
	if (pRow == nullptr)               // disasm: JZ after ADD
		return 0;

	// 4. Level must be > 0 (signed)  (disasm: TEST EDX,EDX ; JG)
	if ((int)dwSkillLevel <= 0)
		return 0;

	// 5. Linear interpolation: (level-1) * perLevelBonus + baseBonus
	//    IMUL is signed -> use signed int multiply to match.
	int nToHitBonusPerLevel = *(int*)(pRow + 0x164);
	int nToHitBonusBase     = *(int*)(pRow + 0x160);
	return ((int)(dwSkillLevel - 1)) * nToHitBonusPerLevel + nToHitBonusBase;
}
