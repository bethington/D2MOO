#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemQualityBracket
extern "C" int __stdcall DATATBLS_GetItemQualityBracket(int nItemType, uint32_t dwLevel)
{
	// g_pExperienceTxtRecords is a pointer variable (name starts g_p_) ->
	// deref the resolved address ONCE to get the table base.
	char* base = (char*)*(void**)D2MOO_Resolve("g_pExperienceTxtRecords");
	if (!base) return -0x7FFFFFFF; // resolver missing / name unknown -> obvious mismatch

	int iVar2;

	if ((nItemType < 0) || (6 < nItemType)) {
		nItemType = 0;
	}
	if ((nItemType < 0) || (iVar2 = nItemType, 6 < nItemType)) {
		iVar2 = 0;
	}

	// &g_pExperienceTxtRecords[1].dwAmazon + nItemType
	//   = base + sizeof(record) + offsetof(dwAmazon) + nItemType*sizeof(uint)
	//   = base + 0x20 + nItemType*4   (record stride 0x20, dwAmazon at offset 0)
	uint32_t* puVar4 = (uint32_t*)(base + 0x20) + nItemType;

	// (&g_pExperienceTxtRecords->dwAmazon)[iVar2]
	//   = base + iVar2*4              (dwAmazon at offset 0, then iVar2-th element)
	uint32_t uVar1 = *puVar4;
	int iVar3 = 0;

	// loop: threshold <= dwLevel AND iVar3 < row_count_for_type(iVar2)
	for (iVar3 = 0;
		 (uVar1 <= dwLevel) && (iVar3 < (int)(*(uint32_t*)(base + iVar2 * 4)));
		 iVar3 = iVar3 + 1)
	{
		uVar1 = puVar4[8];   // stride 0x20 (8 * sizeof(uint)) to next row, same column
		puVar4 = puVar4 + 8;
	}
	return iVar3;
}
