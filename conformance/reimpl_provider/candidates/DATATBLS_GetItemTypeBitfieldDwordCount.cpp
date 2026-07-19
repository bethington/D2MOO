#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemTypeBitfieldDwordCount
extern "C" int __stdcall DATATBLS_GetItemTypeBitfieldDwordCount(void)
{
	void* _g = D2MOO_Resolve("g_pDataTables");
	if (!_g) return 0;
	// g_pDataTables is a POINTER variable -> deref once to get the table base.
	char* base = (char*)*(void**)_g;
	if (!base) return 0;

	// Read item-type count as a SIGNED int at offset +0xC4, then perform the
	// literal CDQ/AND/SAR ceiling-divide-by-32 dance from the decompile.
	uint32_t nBitCountPadded = (uint32_t)(*(int*)(base + 0xc4) + 0x1f);
	return (int)(nBitCountPadded + (((int)nBitCountPadded >> 0x1f) & 0x1fU)) >> 5;
}
