#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_ValidateArrayIndex
extern "C" void __stdcall DATATBLS_ValidateArrayIndex(int nIndex)
{
	// g_pDataTables is a pointer variable (g_p prefix) -> deref ONCE.
	void* pTables = D2MOO_Resolve("g_pDataTables");
	if (!pTables) return;
	char* tablesBase = *(char**)pTables;
	if (!tablesBase) return;

	// Step 2: translate decompile literally using `tablesBase` as the struct base.
	// field_0xbc0 = nOverlayTxtRecordCount (entry count)
	int entryCount = *(int*)(tablesBase + 0xbc0);
	if (nIndex < 0 || nIndex >= entryCount) return;

	// field_0xbbc = pOverlayTxt (array base)
	char* arrayBase = *(char**)(tablesBase + 0xbbc);
	if (!arrayBase) return;

	// Element size per plate comment = 0x84; check computed element != NULL
	if (*(void**)(arrayBase + nIndex * 0x84) == 0) return;

	return;
}
