#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ValidateBinkInstallLocation
extern "C" int __stdcall ValidateBinkInstallLocation(uint32_t dwValue, int unused1, int unused2)
{
	// _g_dwStoredValue -> drop leading underscore -> "g_dwStoredValue"
	// g_dw is a data/array/struct base, so the resolver return IS the base.
	char* base = (char*)D2MOO_Resolve("g_dwStoredValue");
	if (!base)
		return 0; // resolver missing / name unknown -> obvious wrong-value sentinel

	// Literal translation of `_g_dwStoredValue = dwValue;`
	*(uint32_t*)base = dwValue;
	return 1;
}
