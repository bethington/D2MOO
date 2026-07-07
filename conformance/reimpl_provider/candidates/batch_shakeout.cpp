// batch_shakeout.cpp -- pipeline shakeout batch (2026-07-06). Real PD2 D2Common
// leaf functions with DELIBERATELY VARIED ABIs, to exercise/validate the live
// oracle marshaller end to end (fastcall arities 1-3, register + stack args,
// buffer-out params, u32/void returns). All are pure (touch only a caller-
// supplied buffer -> safe to call cold from the oracle thread). Originals are
// resolved by verified offset (Ghidra); these reimpls mirror the decompiled
// behavior exactly.

// GetSeedHi (PD2 0x36700): uint __fastcall(pSeed) -> pSeed->nHighSeed (dword @+4).
// D2MOO_REIMPL_EXPORT: GetSeedHi
extern "C" unsigned int __fastcall GetSeedHi(unsigned int* pSeed)
{
	return pSeed[1];
}

// GetItemRandSeed (PD2 0x36730): uint __fastcall(p) -> first dword.
// D2MOO_REIMPL_EXPORT: GetItemRandSeed
extern "C" unsigned int __fastcall GetItemRandSeed(unsigned int* pRandData)
{
	return pRandData[0];
}

// InitRngSeed (PD2 0x36740): void __fastcall(pState, seed) -> pState[0]=seed,
// pState[1]=0x29a (the LCG multiplier constant 666). fastcall arity-2 + buf out.
// D2MOO_REIMPL_EXPORT: InitRngSeed
extern "C" void __fastcall InitRngSeed(unsigned int* pState, unsigned int dwSeed)
{
	pState[0] = dwSeed;
	pState[1] = 0x29a;
}

// SetCoordPair (PD2 0x36720): void __fastcall(pCoord, x, y) -> pCoord[0]=x,
// pCoord[1]=y. fastcall arity-3: ptr->ECX, x->EDX, y->STACK; buf out.
// D2MOO_REIMPL_EXPORT: SetCoordPair
extern "C" void __fastcall SetCoordPair(unsigned int* pCoord, unsigned int dwX, unsigned int dwY)
{
	pCoord[0] = dwX;
	pCoord[1] = dwY;
}
