#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STATS_TestBitfieldTable_144
extern "C" uint32_t __stdcall STATS_TestBitfieldTable_144(uint32_t dwStatId)
{
	// g_pDataTables is a pointer variable (g_p*) -- resolve, then deref once
	void* _g_pDataTables = D2MOO_Resolve("g_pDataTables");
	if (_g_pDataTables == nullptr) return 0;
	char* base_pDataTables = (char*)*(void**)_g_pDataTables;
	if (base_pDataTables == nullptr) return 0;

	// g_dat_6fdd90b0 IS the game symbol backing the bit-mask table
	// (disasm: MOV EDX,[0x6fdd90b0] then [EDX+ECX*4]) -- pointer variable, deref once
	void* _g_dat_6fdd90b0 = D2MOO_Resolve("g_dat_6fdd90b0");
	if (_g_dat_6fdd90b0 == nullptr) return 0;
	char* base_bitmasks = (char*)*(void**)_g_dat_6fdd90b0;
	if (base_bitmasks == nullptr) return 0;

	// 1. Validate dwStatId is within valid range [0, dwBitCount)
	//    dwBitCount is at offset 0xc4 in DataTables (per disassembly)
	if ((-1 < (int)dwStatId) && ((int)dwStatId < *(int*)(base_pDataTables + 0xc4))) {
		// 2-4. word index = dwStatId >> 5; bit pos = dwStatId & 0x1f
		//    field_0x144 holds the base of the bitfield array; bit mask from g_dat_6fdd90b0[idx]
		return *(uint32_t*)(*(int*)(base_pDataTables + 0x144) + ((int)dwStatId >> 5) * 4) &
		       *(uint32_t*)(base_bitmasks + (dwStatId & 0x1f) * 4);
	}
	return 0;
}
