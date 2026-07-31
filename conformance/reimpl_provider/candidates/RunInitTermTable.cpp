// D2MOO_REIMPL_EXPORT: RunInitTermTable
#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: RunInitTermTable
// Iterates the .CRT$X** init/term function pointer table and calls each non-null
// entry. Mirrors MSVC _initterm(): p = start; p < end; ++p { if (*p) (*p)(); }
// In this build both bounds collapse to the same linker-provided sentinel
// g_pInitTermTableEnd (no entries between them at compile time), so the loop
// is empty -- but the algorithm is preserved exactly so the reimpl is identical
// to the original on every input set.
extern "C" void __stdcall RunInitTermTable(void)
{
	// g_pInitTermTableEnd is a POINTER VARIABLE (name starts g_p). The decompile
	// writes `&g_pInitTermTableEnd` on both sides -- i.e. the address of the
	// symbol itself, NOT the value it holds. Per the mechanical rule, that
	// means: no extra dereference; use the resolved address verbatim.
	uint32_t* table_start = (uint32_t*)D2MOO_Resolve("g_pInitTermTableEnd");
	uint32_t* table_end   = (uint32_t*)D2MOO_Resolve("g_pInitTermTableEnd");
	if (!table_start || !table_end)
		return; // resolver missing / name unknown -> obvious mismatch sentinel

	for (uint32_t* pdwCallbackEntry = table_start;
	     pdwCallbackEntry < table_end;
	     pdwCallbackEntry = pdwCallbackEntry + 1)
	{
		if ((void*)*pdwCallbackEntry != (void*)0x0)
		{
			(*(void(*)(void))*pdwCallbackEntry)();
		}
	}
}
