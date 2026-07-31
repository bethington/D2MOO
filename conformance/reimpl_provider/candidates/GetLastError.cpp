#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetLastError
extern "C" uint32_t __stdcall GetLastError(void)
{
	char* base = (char*)D2MOO_Resolve("g_dwLastError_6fbcc3e0");
	if (!base)
		return 0xFFFFFFFFu;
	return *(uint32_t*)base;
}
