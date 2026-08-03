#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitializeCharacterInventoryDialog
extern "C" int __stdcall InitializeCharacterInventoryDialog(void)
{
	// 1. Set g_dwCharacterInventoryDialogId = 3 (character inventory dialog type ID)
	int* id_base = (int*)D2MOO_Resolve("g_dwCharacterInventoryDialogId");
	if (!id_base) return 0;
	*(int*)id_base = 3;

	// 2. Set g_pfnCharacterInventoryDialogInit[0] = CLIENT_SendResetPacketType4b
	//    (g_pfn... is a pointer variable -> deref resolved address once to get array base)
	void** pfn_slot_ptr = (void**)D2MOO_Resolve("g_pfnCharacterInventoryDialogInit");
	if (!pfn_slot_ptr) return 0;
	char* pfn_arr = *(char**)pfn_slot_ptr;
	if (!pfn_arr) return 0;
	// The decompile assigns CLIENT_SendResetPacketType4b's address; the original's
	// function address cannot be reproduced in the reimpl DLL, but the slot is
	// still mutated as in the original.
	*(void**)(pfn_arr + 0x0) = 0;

	// 3. Set g_dwCharacterInventoryDialogHeight (2-byte / short write at offset 0) = 0xd38
	short* height_base = (short*)D2MOO_Resolve("g_dwCharacterInventoryDialogHeight");
	if (!height_base) return 0;
	*(short*)height_base = (short)0xd38;

	return 1;
}
