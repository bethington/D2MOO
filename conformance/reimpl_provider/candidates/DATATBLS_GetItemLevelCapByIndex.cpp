#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemLevelCapByIndex
extern "C" uint32_t __stdcall DATATBLS_GetItemLevelCapByIndex(int nIndex)
{
	// g_pExperienceTxtRecords is a pointer variable -- deref resolved address ONCE.
	char* base = (char*)*(void**)D2MOO_Resolve("g_pExperienceTxtRecords");
	if (!base)
		return 0xDEADBEEFu; // resolver not injected / name unknown -> obvious mismatch

	// Disasm-derived offsets (plate comment CONF_LIVE 16/16):
	//   count           at base + 0x00
	//   default cap     at base + 0x1c  (used when nIndex <= 0)
	//   per-index cap   at base + nIndex*0x20 + 0x3c  (used when 1 <= nIndex <= count)
	if (nIndex < 1) {
		return *(uint32_t*)(base + 0x1c);
	}
	if (nIndex <= *(int*)(base + 0)) {
		return *(uint32_t*)(base + nIndex * 0x20 + 0x3c);
	}
	return 0;
}
