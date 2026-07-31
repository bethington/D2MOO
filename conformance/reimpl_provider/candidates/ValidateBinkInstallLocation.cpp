// ValidateBinkInstallLocation_reimpl.cpp -- D2MOO reimpl provider.
// The original function is a stub that unconditionally returns TRUE (1).
// No globals are read; no parameters are taken. Reproduce that exactly.

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ValidateBinkInstallLocation
extern "C" int __stdcall ValidateBinkInstallLocation(void)
{
	// Stub: always reports the Bink install location is valid.
	return 1;
}
