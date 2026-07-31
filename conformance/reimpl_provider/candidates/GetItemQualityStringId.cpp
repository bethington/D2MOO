// GetItemQualityStringId.cpp -- D2MOO reimpl provider.
// Pure lookup, no game state. Matches the disassembly exactly (not just the
// decompile -- Ghidra's declared `int` return type undersells the real ABI:
// every path is a 16-bit `MOV AX, imm` / `XOR AX, AX`, upper EAX untouched).
// EAX-input (Class D): `DEC EAX; CMP EAX,0x5` operates on EAX directly, no
// stack load.
//   1 (Normal)  -> 0x10a1
//   2 (Superior)-> 0x10a3
//   3 (Magic)   -> 0x10c3
//   4 (Set)     -> 0x10a2
//   5 (Unique)  -> 0x10a4
//   6 (Rare)    -> 0xdc4
//   else        -> 0

#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: GetItemQualityStringId
extern "C" unsigned int __fastcall GetItemQualityStringId(unsigned int nQuality)
{
	switch (nQuality - 1) {
		case 0: return 0x10a1;
		case 1: return 0x10a3;
		case 2: return 0x10c3;
		case 3: return 0x10a2;
		case 4: return 0x10a4;
		case 5: return 0xdc4;
		default: return 0;
	}
}
