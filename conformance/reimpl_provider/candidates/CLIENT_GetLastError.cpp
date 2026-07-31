#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CLIENT_GetLastError
extern "C" uint32_t __stdcall CLIENT_GetLastError(void)
{
	char* base = (char*)D2MOO_Resolve("g_dwLastError_6fba496c");
	if (!base)
		return 0xDEADBEEFu; // resolver not injected -> obvious mismatch
	return *(uint32_t*)base;
}
