// seed_getrandom.cpp -- register-explicit ABI validation (from the batch shakeout
// finding). PD2's SEED_GetRandomNumber (0x510b0) uses a CUSTOM convention: seed
// pointer in ECX, `max` in EAX (not the fastcall EDX), returns in EAX. The oracle
// calls the ORIGINAL register-explicit (orig_regs in the spec); this reimpl is a
// NORMAL __fastcall(pSeed, max) so the standard marshaller calls it with the same
// logical inputs (pSeed->ECX, max->EDX) -- no hand-asm reimpl needed.
//
// Exact decompiled behavior (D2 LCG, advances the seed in place):
//   if ((int)max < 1) return 0;
//   uVar2 = (u64)low * 0x6AC690C5 + high;  *pSeed(8 bytes) = uVar2;
//   return (max is pow2) ? (max-1) & low32(uVar2) : low32(uVar2) % max;

// D2MOO_REIMPL_EXPORT: SEED_GetRandomNumber
extern "C" unsigned int __fastcall SEED_GetRandomNumber(unsigned int* pSeed, unsigned int max)
{
	if ((int)max < 1)
		return 0;
	unsigned long long lVar1 = (unsigned long long)pSeed[0] * 0x6AC690C5ull;
	unsigned long long uVar2 = lVar1 + pSeed[1];
	pSeed[0] = (unsigned int)uVar2;
	pSeed[1] = (unsigned int)(uVar2 >> 32);
	if ((max & (max - 1)) != 0)
		return (unsigned int)((uVar2 & 0xFFFFFFFFull) % max);
	return (max - 1) & (unsigned int)uVar2;
}
