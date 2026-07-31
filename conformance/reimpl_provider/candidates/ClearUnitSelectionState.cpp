#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ClearUnitSelectionState
extern "C" void __stdcall ClearUnitSelectionState(void)
{
	// 1. Zero g_dwUnitSelectionError (DWORD global) -- data type, resolve directly.
	char* base_err = (char*)D2MOO_Resolve("g_dwUnitSelectionError");
	if (!base_err)
		return;
	*(int*)base_err = 0;

	// 2. Zero g_pLastSelectedUnit (pointer variable) -- write NULL into the variable itself.
	void* p_unit = D2MOO_Resolve("g_pLastSelectedUnit");
	if (!p_unit)
		return;
	*(void**)p_unit = (void*)0;
}
