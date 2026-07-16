#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STORM_GetLastError
extern "C" uint32_t __stdcall STORM_GetLastError(void)
{
	void* _g = D2MOO_Resolve("g_dat_6fdf1468");
	if (_g == nullptr) return 0;
	return *(uint32_t*)_g;
}
