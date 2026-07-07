// unit_field_getters.cpp -- 16th+ production-loop batch (2026-07-07): trivial
// byte-field getters from the UNIT_GetMode neighborhood (D2Common+0x848xx), same
// shape as the already-proven UNIT_GetMode (byte __stdcall(UnitAny*) -> byte @
// fixed offset). Confirmed from disassembly (asm is ground truth): arg at
// [ESP+4], RET 0x4 (stdcall), return in AL.

// GetUnitField91 (PD2 0x34860): byte __stdcall(UnitAny* pUnit) -> *(pUnit+0x91).
// D2MOO_REIMPL_EXPORT: GetUnitField91
extern "C" unsigned char __stdcall GetUnitField91(void* pUnit)
{
	return *((unsigned char*)pUnit + 0x91);
}

// GetByte0x94 (PD2 0x34620): byte __stdcall(void* pBlock) -> *(pBlock+0x94).
// Compiler-helper-style leaf, same fixed-offset-byte-read shape as GetUnitField91.
// D2MOO_REIMPL_EXPORT: GetByte0x94
extern "C" unsigned char __stdcall GetByte0x94(void* pBlock)
{
	return *((unsigned char*)pBlock + 0x94);
}

// SetByte0x93Validated (PD2 0x346a0): void __stdcall(void* pStruct, byte bValue).
// Null-safe, no abort path (unlike the sibling SetValidatedByte0x94): bValue in
// [1,19] stores (bValue-1) at +0x93 (1-based -> 0-based); anything else
// (including 0, >=20, or pStruct==NULL) stores 0 / no-ops. Confirmed from
// disassembly: both args on stack, RET 0x8 (stdcall).
// D2MOO_REIMPL_EXPORT: SetByte0x93Validated
extern "C" void __stdcall SetByte0x93Validated(void* pStruct, unsigned char bValue)
{
	if (!pStruct) return;
	*((unsigned char*)pStruct + 0x93) = (bValue != 0 && bValue < 0x14) ? (unsigned char)(bValue - 1) : 0;
}
