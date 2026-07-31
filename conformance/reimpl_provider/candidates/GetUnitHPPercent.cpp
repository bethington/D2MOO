// D2MOO reimpl of GetUnitHPPercent -- walks g_pRosterPetList linked list
// looking for entry whose +0x08 dwUnitGuid matches dwUnitGuid (ECX), and
// returns the +0x1c nHPPercent. Returns 0 if not found. Live game state
// is read by NAME through the injected resolver (no hardcoded addresses).

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetUnitHPPercent
extern "C" int __fastcall GetUnitHPPercent(uint32_t dwUnitGuid)
{
	// g_pRosterPetList is a pointer variable (g_p prefix): resolve gives &ptr, deref once.
	void* resolved = D2MOO_Resolve("g_pRosterPetList");
	if (!resolved)
		return -1; // resolver not injected -- obvious wrong-value sentinel

	char* pPetEntry = *(char**)resolved;

	// Walk the linked list: dwUnitGuid at +0x08, pNext at +0x30, nHPPercent at +0x1c.
	while (pPetEntry != (char*)0x0) {
		if (*(uint32_t*)(pPetEntry + 0x08) == dwUnitGuid)
			break;
		pPetEntry = *(char**)(pPetEntry + 0x30);
	}

	if (pPetEntry == (char*)0x0)
		return 0;

	return *(int*)(pPetEntry + 0x1c);
}
