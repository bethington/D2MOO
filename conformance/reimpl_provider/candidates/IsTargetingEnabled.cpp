#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: IsTargetingEnabled
extern "C" int __stdcall IsTargetingEnabled(void)
{
	// g_pPlayerName_6fbcc3fc is a pointer variable (name starts with g_p_):
	// D2MOO_Resolve returns the ADDRESS OF THE SYMBOL (i.e. &g_pPlayerName_6fbcc3fc),
	// so deref once to obtain the actual pointer value being tested.
	void* symAddr = (void*)D2MOO_Resolve("g_pPlayerName_6fbcc3fc");
	if (!symAddr)
		return -1; // resolver not injected / name unknown -> obvious mismatch

	char* val = *(char**)symAddr;
	return val != (char*)0x0 ? 1 : 0;
}
