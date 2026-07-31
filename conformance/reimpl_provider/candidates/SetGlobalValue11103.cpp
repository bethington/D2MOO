#include "../provider_runtime.h"

// NEEDS GLOBAL: g_dwValue11103

// D2MOO_REIMPL_EXPORT: SetGlobalValue11103
// Sets a module-level global value (ordinal 11103).
// dwValue (uint, in ECX) -> stored to g_dwValue11103; void return.
extern "C" void __fastcall SetGlobalValue11103(uint32_t dwValue)
{
	// Data variable (g_dw... prefix, not g_p...) -> resolver returns the address of the storage directly.
	char* base = (char*)D2MOO_Resolve("g_dwValue11103");
	if (!base)
		return; // void function; resolver missing => nothing to set, proof will mismatch on the global's value
	*(uint32_t*)base = dwValue;
}
