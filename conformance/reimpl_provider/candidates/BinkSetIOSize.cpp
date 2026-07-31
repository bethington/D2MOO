#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: BinkSetIOSize
extern "C" void __stdcall BinkSetIOSize(uint32_t dwIOSize)
{
	// NEEDS GLOBAL: g_dwBinkIOSize
	uint32_t* p = (uint32_t*)D2MOO_Resolve("g_dwBinkIOSize");
	if (!p) return;
	*p = dwIOSize;
}
