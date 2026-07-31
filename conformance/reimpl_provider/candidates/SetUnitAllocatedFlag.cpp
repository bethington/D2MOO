#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: SetUnitAllocatedFlag
// NEEDS GLOBAL: g_dwPrimaryTemplateDebugEnabled
extern "C" void __stdcall SetUnitAllocatedFlag(int nUnitType)
{
	uint32_t* base = (uint32_t*)D2MOO_Resolve("g_dwPrimaryTemplateDebugEnabled");
	if (!base)
		return;
	*base = (nUnitType == 0) ? 1u : 0u;
}
