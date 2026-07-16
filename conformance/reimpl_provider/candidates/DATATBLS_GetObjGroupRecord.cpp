#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: DATATBLS_GetObjGroupRecord
// Hand-proof 2026-07-14: the ObjGroup globals are NOT in the resolve table
// (dat_global gap). D2Common is at its preferred base in this game, so the
// proof reads them by absolute address: count @0x6fdf0ba0, table @0x6fdf0b9c
// (from the disasm: CMP [0x6fdf0ba0],EAX / MOV ECX,[0x6fdf0b9c]).
extern "C" void* __stdcall DATATBLS_GetObjGroupRecord(uint32_t dwIndex)
{
	// signed bounds check, exactly as the original: JLE on (count <= idx)
	if ((int)dwIndex < *(int*)0x6fdf0ba0) {
		// no NULL check on the table base in the original -- keep it that way
		return (void*)(*(char**)0x6fdf0b9c + dwIndex * 0x34);
	}
	return (void*)0;
}
