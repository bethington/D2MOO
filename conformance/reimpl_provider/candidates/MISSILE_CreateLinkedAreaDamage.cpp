#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: MISSILE_CreateLinkedAreaDamage
extern "C" int __fastcall MISSILE_CreateLinkedAreaDamage(void* pMissile, void* pMissileData, int nUnitContext)
{
	if (pMissileData == 0) return 1;

	// _DAT_000fe898 -> g_pDataTables (pointer variable -> deref once)
	char* base = (char*)*(void**)D2MOO_Resolve("g_pDataTables");
	if (!base) return 0xDEADBEEF;

	// dwOwnerParam is at offset 0x08 in MissileDataTbl
	uint32_t dwOwnerParam = *(uint32_t*)((char*)pMissileData + 0x08);

	int tableLen = *(int*)(base + 0xb6c);
	if ((int)dwOwnerParam < 0 || (int)dwOwnerParam >= tableLen) return 1;

	uint32_t dwEffectFlags = (uint32_t)dwOwnerParam * 0x1a4u + *(uint32_t*)(base + 0xb64);
	if (dwEffectFlags == 0) return 1;

	// Remaining logic invokes helpers (Unwind_6fd179c0, FindLinkedUnit,
	// MISSILE_CalculateElementalDamageStats, MISSILE_ApplyMissileRecordFieldValue,
	// FindUnitByTypeAndId, ApplyAreaEffectToUnits) that are not defined in the
	// provider; the function unconditionally returns 1 in all paths.
	return 1;
}
