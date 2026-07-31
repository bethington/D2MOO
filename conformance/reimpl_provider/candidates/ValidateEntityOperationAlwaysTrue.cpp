// ValidateEntityOperationAlwaysTrue.cpp -- stub validator that always returns TRUE.
// Live conformance reimpl: trivial leaf, no globals read, no inputs.
// The plate comment says it lives in the g_aValidateEntityOperations dispatch
// table and unconditionally returns 1. Because the function takes no parameters
// and reads no game state, the reimpl does not need to resolve any global --
// both the original and the reimpl simply return 1 for every call.

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: ValidateEntityOperationAlwaysTrue
extern "C" int __stdcall ValidateEntityOperationAlwaysTrue(void)
{
	return 1;
}
