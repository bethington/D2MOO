#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: TranslateStateBits
extern "C" uint32_t __stdcall TranslateStateBits(uint32_t dwStateBits)
{
	uint32_t dwRemappedBits;
	uint32_t dwField300;

	dwRemappedBits = (uint32_t)((dwStateBits & 0x10) != 0);
	if ((dwStateBits & 8) != 0) {
		dwRemappedBits = dwRemappedBits | 4;
	}
	if ((dwStateBits & 4) != 0) {
		dwRemappedBits = dwRemappedBits | 8;
	}
	if ((dwStateBits & 2) != 0) {
		dwRemappedBits = dwRemappedBits | 0x10;
	}
	if ((dwStateBits & 1) != 0) {
		dwRemappedBits = dwRemappedBits | 0x20;
	}
	if ((dwStateBits & 0x80000) != 0) {
		dwRemappedBits = dwRemappedBits | 2;
	}
	dwField300 = dwStateBits & 0x300;
	if (dwField300 != 0) {
		if (dwField300 == 0x100) {
			dwRemappedBits = dwRemappedBits | 0x400;
		}
		else if (dwField300 == 0x200) {
			dwRemappedBits = dwRemappedBits | 0x800;
		}
		else if (dwField300 == 0x300) {
			dwRemappedBits = dwRemappedBits | 0xc00;
		}
	}
	if ((dwStateBits & 0x30000) == 0) {
		dwRemappedBits = dwRemappedBits | 0x300;
	}
	else if ((dwStateBits & 0x30000) == 0x10000) {
		dwRemappedBits = dwRemappedBits | 0x200;
	}
	if ((dwStateBits & 0x40000) != 0) {
		dwRemappedBits = dwRemappedBits | 0x1000;
	}
	return dwRemappedBits;
}
