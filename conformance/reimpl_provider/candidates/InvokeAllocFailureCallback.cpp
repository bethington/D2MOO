#include "../provider_runtime.h"

// NEEDS GLOBAL: g_pfnAllocFailureCallback

// D2MOO_REIMPL_EXPORT: InvokeAllocFailureCallback
extern "C" uint32_t __cdecl InvokeAllocFailureCallback(uint32_t dwSize)
{
	// Per the AUTHORITATIVE ABI: callee cleans 0 bytes (RET 0x0) -> 0 stack
	// parameters. dwSize is therefore passed in EAX, not on the stack.
	void** base = (void**)D2MOO_Resolve("g_pfnAllocFailureCallback");
	if (!base)
		return 0u; // resolver missing / global unknown -> obvious wrong-value sentinel

	// g_pfnAllocFailureCallback is a function-pointer variable (g_pfn*).
	// Resolve returns the ADDRESS OF THE SYMBOL; one dereference yields the
	// function pointer value the decompile uses as bare _g_pfnAllocFailureCallback.
	void* pfn = *base;
	if (pfn != (void*)0)
	{
		int nResult = ((int (__cdecl*)(uint32_t))pfn)(dwSize);
		if (nResult != 0)
			return 1u;
	}
	return 0u;
}
