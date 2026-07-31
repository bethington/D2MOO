#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: CODEPAGE_CheckCharType
// NEEDS GLOBAL: g_pHexDigitLookup

extern "C" int __stdcall CODEPAGE_GetStringType(int, const char*, int, uint16_t*, int, int, int);

extern "C" uint32_t __stdcall CODEPAGE_CheckCharType(int nCharCode, uint32_t dwTypeMask)
{
	// g_pHexDigitLookup is a pointer variable (g_p prefix) -> deref once
	char* base = (char*)*(void**)D2MOO_Resolve("g_pHexDigitLookup");
	if (!base) return 0xDEADBEEFu; // resolver missing

	// Local reused var: nCharCode upper 16 bits hold the output character type
	uint32_t nCharCodeVar = (uint32_t)nCharCode;

	if ((uint32_t)nCharCode + 1u < 0x101u) {
		// ASCII range: lookup short entry, store in upper 16 bits
		uint16_t lookup = *(uint16_t*)(base + nCharCode * 2);
		nCharCodeVar = (nCharCodeVar & 0x0000FFFFu) | ((uint32_t)lookup << 16);
	} else {
		uint32_t abCharBuf;
		int nSrcLength;

		// Check lead-byte flag at g_pHexDigitLookup[(nCharCode>>8)&0xFF] high byte
		if ((*(uint8_t*)(base + ((nCharCodeVar >> 8) & 0xFFu) * 2 + 1) & 0x80u) == 0) {
			// Single-byte: low byte of nCharCode, mask out bits 8-15
			abCharBuf = ((uint32_t)(uint8_t)nCharCode) & 0xFFFF00FFu;
			nSrcLength = 1;
		} else {
			// Two-byte: low 16 bits of nCharCode, mask out bits 16-23
			abCharBuf = ((uint32_t)(uint8_t)nCharCode) | (((uint32_t)(uint8_t)(nCharCode >> 8)) << 8);
			abCharBuf &= 0xFF00FFFFu;
			nSrcLength = 2;
		}

		int ret = CODEPAGE_GetStringType(1, (const char*)&abCharBuf, nSrcLength, (uint16_t*)((char*)&nCharCodeVar + 2), 0, 0, 1);
		if (ret == 0) return 0;
	}

	return (nCharCodeVar >> 16) & dwTypeMask;
}
