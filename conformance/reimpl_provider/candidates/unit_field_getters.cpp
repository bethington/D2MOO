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

// GetUnitFlag2 (PD2 0x349c0): uint __stdcall(void* pData) -> *(pData+0x34) & 2.
// D2MOO_REIMPL_EXPORT: GetUnitFlag2
extern "C" unsigned int __stdcall GetUnitFlag2(void* pData)
{
	return *(unsigned int*)((unsigned char*)pData + 0x34) & 2u;
}

// STAT_GetStatListFlag2 (PD2 0x34950): uint __stdcall(D2StatListStrc* pStatList)
// -> *(pStatList+0x34) & 4. Same fixed-offset-flags shape as GetUnitFlag2, mask 4.
// D2MOO_REIMPL_EXPORT: STAT_GetStatListFlag2
extern "C" unsigned int __stdcall STAT_GetStatListFlag2(void* pStatList)
{
	return *(unsigned int*)((unsigned char*)pStatList + 0x34) & 4u;
}

// GetStructFlag0x20 (PD2 0x34c70): uint __stdcall(void* pStruct) -> *(pStruct+0x34)
// & 0x20. Third sibling of the +0x34 flags-dword family (GetUnitFlag2 mask=2,
// STAT_GetStatListFlag2 mask=4, this one mask=0x20).
// D2MOO_REIMPL_EXPORT: GetStructFlag0x20
extern "C" unsigned int __stdcall GetStructFlag0x20(void* pStruct)
{
	return *(unsigned int*)((unsigned char*)pStruct + 0x34) & 0x20u;
}

// SetUnitFlag0x20 (PD2 0x34c80): void __stdcall(void* pUnit, int fEnable) --
// sets/clears bit 0x20 in the SAME +0x34 flags dword the getters above read.
// D2MOO CAUTION (2026-07-07): reimplemented but DEFERRED from live oracle/shadow
// proving, same reason as UNIT_SetStructByte0x90 -- testing against the live
// captured unit handle would flip a real flag bit on the currently-played
// character mid-session. Needs a read-verify-restore oracle protocol first.
// D2MOO_REIMPL_EXPORT: SetUnitFlag0x20
extern "C" void __stdcall SetUnitFlag0x20(void* pUnit, int fEnable)
{
	unsigned int* p = (unsigned int*)((unsigned char*)pUnit + 0x34);
	*p = fEnable != 0 ? (*p | 0x20u) : (*p & 0xffffffdfu);
}

// GetStructField0x04 (PD2 0x34b20): uint __stdcall(void* pStruct) -> *(pStruct+4).
// Aborts (CleanupAndAbort) if pStruct==NULL; SAFE to shadow/oracle-test only with
// a guaranteed-non-null pointer (e.g. the live captured unit handle, as here).
// D2MOO_REIMPL_EXPORT: GetStructField0x04
extern "C" unsigned int __stdcall GetStructField0x04(void* pStruct)
{
	return *(unsigned int*)((unsigned char*)pStruct + 4);
}

// GetFirstDwordOrAbort (PD2 0x34b50): uint __stdcall(void* pStruct) -> *pStruct
// (offset 0). Same abort-on-NULL shape as GetStructField0x04; same caution.
// D2MOO_REIMPL_EXPORT: GetFirstDwordOrAbort
extern "C" unsigned int __stdcall GetFirstDwordOrAbort(void* pStruct)
{
	return *(unsigned int*)pStruct;
}

// GetUnitField88 (PD2 0x34cd0): uint __stdcall(void* pUnit) -> *(pUnit+0x88).
// D2MOO_REIMPL_EXPORT: GetUnitField88
extern "C" unsigned int __stdcall GetUnitField88(void* pUnit)
{
	return *(unsigned int*)((unsigned char*)pUnit + 0x88);
}

// ROSTER_GetXPos (PD2 0x34c60): uint __stdcall(RosterUnit* pRosterUnit) ->
// *(pRosterUnit+0x28). Different struct type than the live captured unit
// handle, but memory-safe to test against it anyway (same fixed-offset-read
// shape; proves the FUNCTION's offset/ABI, not the field's semantic meaning).
// D2MOO_REIMPL_EXPORT: ROSTER_GetXPos
extern "C" unsigned int __stdcall ROSTER_GetXPos(void* pRosterUnit)
{
	return *(unsigned int*)((unsigned char*)pRosterUnit + 0x28);
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
