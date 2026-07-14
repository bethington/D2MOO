#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STATS_TestBitfieldTable_FC
extern "C" uint32_t __stdcall STATS_TestBitfieldTable_FC(uint32_t dwStatId)
{
	// g_pDataTables: pointer variable (g_p prefix + disasm MOV EAX,[global] then deref) -> deref once
	void* _g_pDataTables = D2MOO_Resolve("g_pDataTables");
	if (!_g_pDataTables) return 0;
	char* base_DataTables = (char*)*(void**)_g_pDataTables;
	if (!base_DataTables) return 0;

	// g_dat_6fdd90b0 (= g_pdwStatBitMasks): pointer variable (disasm MOV EDX,[0x6fdd90b0] then [EDX+ECX*4]) -> deref once
	void* _g_pdwStatBitMasks = D2MOO_Resolve("g_dat_6fdd90b0");
	if (!_g_pdwStatBitMasks) return 0;
	char* base_pdwStatBitMasks = (char*)*(void**)_g_pdwStatBitMasks;
	if (!base_pdwStatBitMasks) return 0;

	// dwBitCount at offset 0xC4 (confirmed via CMP ECX,[EAX+0xc4])
	uint32_t dwBitCount = *(uint32_t*)(base_DataTables + 0xC4);

	// Range check: (-1 < (int)dwStatId) && ((int)dwStatId < (int)dwBitCount)
	if ((int)dwStatId < 0 || (int)dwStatId >= (int)dwBitCount) {
		return 0;
	}

	// pBitfieldTable at offset 0xFC (confirmed via MOV EAX,[EAX+0xfc])
	char* pBitfieldTable = *(char**)(base_DataTables + 0xFC);
	if (!pBitfieldTable) return 0;

	// (dwStatId >> 5) * 4 = DWORD index into bitfield array, scaled by 4 bytes
	// (dwStatId & 0x1F) * 4 = bit position 0-31, scaled by 4 bytes
	return *(uint32_t*)(pBitfieldTable + ((int)dwStatId >> 5) * 4) &
	       *(uint32_t*)(base_pdwStatBitMasks + (dwStatId & 0x1F) * 4);
}
