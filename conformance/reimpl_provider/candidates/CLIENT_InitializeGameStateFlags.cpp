// CLIENT_InitializeGameStateFlags_reimpl.cpp
// Live-conformance reimpl: writes six client-side state globals to their
// cleared/null sentinel values. Triggered at game startup.

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_InitializeGameStateFlags
extern "C" void __stdcall CLIENT_InitializeGameStateFlags(void)
{
	// Resolve each global by name. g_dw* are scalar DWORDs (write directly
	// to the resolved address). g_pLastSelectedUnit is a pointer variable
	// (g_p*), so the resolved address is the location of the pointer slot
	// itself -- write NULL to that slot.

	uint32_t* pSelMonster = (uint32_t*)D2MOO_Resolve("g_dwSelectedMonsterIndex");
	if (!pSelMonster) return;
	*pSelMonster = 0xffffffffu;

	uint32_t* pSelItemSlot = (uint32_t*)D2MOO_Resolve("g_dwSelectedItemSlot");
	if (!pSelItemSlot) return;
	*pSelItemSlot = 0xffffffffu;

	uint32_t* pDispWin = (uint32_t*)D2MOO_Resolve("g_dwDisplayWindowHandle");
	if (!pDispWin) return;
	*pDispWin = 0xffffffffu;

	uint32_t* pMonVis = (uint32_t*)D2MOO_Resolve("g_dwMonsterVisible");
	if (!pMonVis) return;
	*pMonVis = 0u;

	uint32_t* pSelErr = (uint32_t*)D2MOO_Resolve("g_dwUnitSelectionError");
	if (!pSelErr) return;
	*pSelErr = 0u;

	void** pLastUnit = (void**)D2MOO_Resolve("g_pLastSelectedUnit");
	if (!pLastUnit) return;
	*pLastUnit = (void*)0;
}
