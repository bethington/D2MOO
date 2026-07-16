#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: STATS_GetTableValue64
// Hand-proof 2026-07-14: register-explicit original (index in EAX, no stack
// args -- disasm: CMP EAX,0x40 / JC / MOV EAX,[EAX*4+0x6fde3200]). The oracle
// calls the original via orig_regs; this reimpl is the normal __fastcall
// counterpart. Bounds check is UNSIGNED (JC=JB). Table @0x6fde3200 read by
// absolute address (static data, D2Common at preferred base).
extern "C" uint32_t __fastcall STATS_GetTableValue64(uint32_t nIndex)
{
	if (nIndex >= 0x40) {
		return 0;
	}
	return ((uint32_t*)0x6fde3200)[nIndex];
}
