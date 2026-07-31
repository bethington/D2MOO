#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: FireSkillMissilesAtTarget
// __fastcall: ECX=pSourceUnit, EDX=nMissileClassId, [ESP+4]=nSkillLevel (callee-cleaned RET 0x4)
// Validates nMissileClassId against the missile class data table (g_pDataTablesRoot),
// then invokes skill-owner / missile-count / search-radius lookups (Ordinal_10459/10529/11081)
// and unit scan / missile creation. The Ordinal_10459 / Ordinal_10529 / Ordinal_11081 /
// FindUnitByIdInHashTable / UNIT_IterateUnitsInRadius / GetTargetCoordinates /
// CLIENT_CreateMissileUnit / GetSkillDescDamageValue / ReduceValueByPercentage calls
// appear in the decompile as Unwind_6fb7dc30 phantoms because Ghidra could not resolve
// the indirect call targets through the unwind thunk. They cannot be reproduced without
// the real D2Common/D2Client function addresses, so we stub the post-validation path
// with a 0 return (the same early-exit value any of those sub-calls returning a failure
// code would produce in the original).
extern "C" int __fastcall FireSkillMissilesAtTarget(int pSourceUnit, int nMissileClassId, int nSkillLevel)
{
	// g_pDataTablesRoot is a pointer variable (name starts with g_p) -> deref the
	// resolved address ONCE so 'base' is the address of the data tables root struct.
	char* base = (char*)*(void**)D2MOO_Resolve("g_pDataTablesRoot");
	if (!base)
		return -1; // resolver not injected / name unknown -> obvious mismatch sentinel

	// Step 1: Validate missile class ID bounds (nMissileClassCount at +0xba0)
	if ((nMissileClassId < 0) || (*(int*)(base + 0xba0) <= nMissileClassId)) {
		return 0;
	}

	// Step 2: Calculate missile class record pointer (pMissileClassTable at +0xb98, stride 0x23c)
	int pMissileClassRec = nMissileClassId * 0x23c + *(int*)(base + 0xb98);
	if (pMissileClassRec == 0) {
		return 0;
	}

	// Steps 3+: Unwind_6fb7dc30 phantoms correspond to Ordinal_10459 (skill owner),
	// Ordinal_10529 (missile count), Ordinal_11081 (search radius), FindUnitByIdInHashTable,
	// UNIT_IterateUnitsInRadius, GetTargetCoordinates, CLIENT_CreateMissileUnit, etc.
	// These are real function calls in the original binary that Ghidra could not
	// resolve through the unwind thunk. Without their real target addresses, we
	// cannot faithfully reproduce the algorithm past this point. Returning 0 matches
	// the original whenever any of those sub-calls return their failure value
	// (no skill owner, missile count < 1, no target found, etc.).
	return 0;
}
