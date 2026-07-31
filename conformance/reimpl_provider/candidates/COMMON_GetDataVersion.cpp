// COMMON_GetDataVersion reimpl -- D2Common data-table version constant getter.
// The original function unconditionally returns the constant 2; it reads no
// globals and takes no parameters. Reimpl must reproduce that exact behavior.

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: COMMON_GetDataVersion
extern "C" int __stdcall COMMON_GetDataVersion(void)
{
	// No globals read; no params; original returns the literal constant 2.
	return 2;
}
