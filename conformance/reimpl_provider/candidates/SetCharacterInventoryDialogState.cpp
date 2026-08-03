#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SetCharacterInventoryDialogState
extern "C" uint32_t __stdcall SetCharacterInventoryDialogState(void)
{
	char* panelStateA = (char*)D2MOO_Resolve("g_dwPanelStateA");
	char* createParty = (char*)D2MOO_Resolve("g_pfnCreatePartyStatsDialog");
	char* charDialog = (char*)D2MOO_Resolve("g_wCharacterDialogResId");

	if (!panelStateA || !createParty || !charDialog)
		return 0u; // resolver missing -> obvious mismatch sentinel

	*(int*)panelStateA = 3;
	*(void**)createParty = (void*)0x6FAF9440; // CLIENT_CreatePartyStatsDialog
	*(short*)charDialog = (short)0xD45;

	return 1u;
}
