#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetQuestInfo
extern "C" uint32_t __stdcall GetQuestInfo()
{
	// _g_dwLastNpcDialogStringId is a data global (g_dw* prefix, not a pointer-variable).
	// STEP 1: resolve by verified NAME; use the resolved address directly as the base.
	char* base = (char*)D2MOO_Resolve("g_dwLastNpcDialogStringId");
	if (!base)
		return 0xFFFFFFFFu; // resolver missing / name unknown -> obvious mismatch sentinel

	// STEP 2: translate the decompile literally. The decompile reads `_g_dwLastNpcDialogStringId`
	// as a plain DWORD value (no field offset), so we dereference the resolved base once.
	return *(uint32_t*)(base);
}
