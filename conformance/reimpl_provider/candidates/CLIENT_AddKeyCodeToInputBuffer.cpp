#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_AddKeyCodeToInputBuffer
extern "C" void __fastcall CLIENT_AddKeyCodeToInputBuffer(uint16_t wKeyCode)
{
	// g_dwDialogOptionCount is a 4-byte DWORD but only its low 2 bytes are read/written
	// (Ghidra shows (ushort)g_dwDialogOptionCount and g_dwDialogOptionCount._0_2_).
	char* baseCount = (char*)D2MOO_Resolve("g_dwDialogOptionCount");
	if (!baseCount) return;

	// g_wDialogOptionNpcType0 is the start of a 9-entry array of ushort (stride 2).
	char* baseTypes = (char*)D2MOO_Resolve("g_wDialogOptionNpcType0");
	if (!baseTypes) return;

	// Read the low 2 bytes (treat the 4-byte storage as a ushort at offset 0).
	uint16_t count = *(uint16_t*)(baseCount + 0);

	if (count < 9) {
		// Store wKeyCode at the current index (ushort array: stride = 2 bytes).
		*(uint16_t*)(baseTypes + count * 2) = wKeyCode;
		// Increment only the low 2 bytes of the count.
		*(uint16_t*)(baseCount + 0) = count + 1;
	}
}
