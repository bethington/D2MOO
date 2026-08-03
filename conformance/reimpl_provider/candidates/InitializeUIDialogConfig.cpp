#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: InitializeUIDialogConfig
extern "C" int __stdcall InitializeUIDialogConfig(void)
{
	// 1. Set dialog panel state to active (0x03)
	int* pDialogPanelState = (int*)D2MOO_Resolve("g_dwDialogPanelState");
	if (!pDialogPanelState) return 0;
	*pDialogPanelState = 3;

	// 2. Store dialog initialization handler pointer (0x6faf88d0)
	void** pfnDialogInitHandler = (void**)D2MOO_Resolve("g_pfnDialogInitHandler");
	if (!pfnDialogInitHandler) return 0;
	*pfnDialogInitHandler = (void*)0x6faf88d0;

	// 3. Configure dialog panel width = 0x0d36
	short* pwDialogWidth = (short*)D2MOO_Resolve("g_wDialogWidth");
	if (!pwDialogWidth) return 0;
	*pwDialogWidth = (short)0x0d36;

	return 1;
}
