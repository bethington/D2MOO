#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: FindRosterEntryByParam
extern "C" void* __fastcall FindRosterEntryByParam(uint32_t dwUnitGuid)
{
	void* sym = D2MOO_Resolve("g_pRosterPetList");
	if (!sym)
		return (void*)0xDEADBEEF;

	char* pRosterEntry = (char*)*(void**)sym;

	while (pRosterEntry != (char*)0)
	{
		if (*(uint32_t*)(pRosterEntry + 0x8) == dwUnitGuid)
			return pRosterEntry;
		pRosterEntry = *(char**)(pRosterEntry + 0x30);
	}
	return (void*)0;
}
