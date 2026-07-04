# PD2-S12 RNG conformance — read-only confirmation

Proven **read-only** (Frida `Memory.scanSync` + `Instruction.parse`, no function
calls) that PD2-S12's RNG is the same LCG as D2MOO (`D2Seed.h`).

**D2MOO reference** (`source/D2Common/include/D2Seed.h`):
```c
uint64_t lSeed = pSeed->nHighSeed + 0x6AC690C5i64 * pSeed->nLowSeed;
// seed struct: { uint32 nLowSeed @0; uint32 nHighSeed @4; }   __fastcall(pSeed)
```

**PD2-S12 live capture** (D2Common @ runtime base `0x6FD50000`): the magic
multiplier `0x6AC690C5` appears at 10 sites (the inlined `SEED_Roll*` variants).
Representative disassembly at `0x6FD510C0`:
```
mov eax, dword ptr [ecx]        ; eax = pSeed->nLowSeed   (ecx = pSeed => __fastcall)
mov edx, 0x6ac690c5             ; edx = 0x6AC690C5 (LCG multiplier)
mul edx                         ; edx:eax = nLowSeed * 0x6AC690C5   (64-bit)
mov edi, dword ptr [ecx + 4]    ; edi = pSeed->nHighSeed
add eax, edi                    ; low32  += nHighSeed
adc edx, ebx                    ; high32 += carry
```
Matches D2MOO exactly: multiplier `0x6AC690C5`, seed layout `{low@0, high@4}`,
64-bit LCG `nHighSeed + 0x6AC690C5 * nLowSeed`, `pSeed` in ECX (`__fastcall`).
The `lea edi,[esi-1]; test esi,edi; je` sites are the limited/power-of-2 variant
(`SEED_RollLimitedRandomNumber`).

**Status:** RNG *algorithm* conformance PROVEN (structural, read-only). Full
behavioral I/O capture (calling with chosen seeds for `(seed -> sequence)`
golden vectors) is deferred until we have the pure-LCG entry from the PD2-S12
Ghidra DB -- calling a heuristically-located entry crashed the game once, so we
only call Ghidra-VERIFIED addresses. See pd2_frida_rng_probe.js (read-only).
