// D2Debugger.oracle.cpp -- WS-5 "design detail B": the general arbitrary-ABI
// call marshaller for the direct-call oracle (GRADUATED_CONFORMANCE_PIPELINE_PLAN.md).
//
// The coord-family oracle (D2Debugger.LiveDispatch.cpp POST /call/{i}) hard-codes
// one signature: void __stdcall(int*,int*). To prove ARBITRARY D2MOO reimpls
// live we need to call a function whose calling convention and arity are only
// known at runtime (from a JSON spec fun-doc emits). This file is the tiny FFI
// that does exactly that for the x86 32-bit-slot case that covers the vast
// majority of D2 functions: every argument is a 32-bit slot (int / unsigned /
// pointer), passed positionally, for cdecl / stdcall / fastcall / thiscall.
//
// WHY NOT HAND-ASM: a marshaller bug here would call game code with a corrupt
// stack -- so instead of a naked-asm pusher we let the COMPILER generate the
// correct call sequence + stack cleanup per convention, by switching on arity
// and casting `fn` to the exact function-pointer type. Verbose (4 conventions x
// arities 0..8) but each case is provably correct by construction. Wider args
// (float/double/int64/struct-by-value) and >8 args are deferred (see plan).
//
// x86 ONLY. The whole project is 32-bit (CLAUDE.md), so `uint32_t` == one slot.
#include <cstdint>

// One 32-bit argument slot. Widening/pointer args all fit here on x86.
using U = uint32_t;

// Generates the arity switch for one calling convention CONV. Each case casts
// `fn` to a function pointer of the right convention + arity and forwards the
// slots, so the compiler emits the correct argument passing (ecx/edx for
// fast/thiscall) and stack cleanup (caller for cdecl, callee otherwise).
#define D2ORACLE_CALL_CASES(CONV)                                                  \
	switch (n)                                                                      \
	{                                                                              \
	case 0: return ((U(CONV*)())fn)();                                             \
	case 1: return ((U(CONV*)(U))fn)(a[0]);                                        \
	case 2: return ((U(CONV*)(U, U))fn)(a[0], a[1]);                               \
	case 3: return ((U(CONV*)(U, U, U))fn)(a[0], a[1], a[2]);                      \
	case 4: return ((U(CONV*)(U, U, U, U))fn)(a[0], a[1], a[2], a[3]);             \
	case 5: return ((U(CONV*)(U, U, U, U, U))fn)(a[0], a[1], a[2], a[3], a[4]);    \
	case 6: return ((U(CONV*)(U, U, U, U, U, U))fn)(                               \
		a[0], a[1], a[2], a[3], a[4], a[5]);                                       \
	case 7: return ((U(CONV*)(U, U, U, U, U, U, U))fn)(                            \
		a[0], a[1], a[2], a[3], a[4], a[5], a[6]);                                 \
	case 8: return ((U(CONV*)(U, U, U, U, U, U, U, U))fn)(                         \
		a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);                           \
	default: return 0;                                                             \
	}

// Calling conventions understood by the oracle spec. Keep in sync with the
// string parse in D2Debugger.LiveDispatch.cpp (ParseCallConv).
enum D2OracleCC { D2CC_CDECL = 0, D2CC_STDCALL = 1, D2CC_FASTCALL = 2, D2CC_THISCALL = 3 };

// Call `fn` (convention `cc`, `n` 32-bit slots in `a`, 0<=n<=8) and return its
// 32-bit result (EAX). void / pointer / smaller-int returns all come back here;
// callers ignore it when the spec's return type is void. A faulting call is
// expected to be caught by the HTTP server's SEH wrapper (SafeHandle), never
// crashing the game.
extern "C" uint32_t D2Oracle_Call(void* fn, int cc, const uint32_t* a, int n)
{
	if (!fn || n < 0 || n > 8)
		return 0;
	switch (cc)
	{
	case D2CC_CDECL:    D2ORACLE_CALL_CASES(__cdecl)
	case D2CC_STDCALL:  D2ORACLE_CALL_CASES(__stdcall)
	case D2CC_FASTCALL: D2ORACLE_CALL_CASES(__fastcall)
	case D2CC_THISCALL: D2ORACLE_CALL_CASES(__thiscall)
	default: return 0;
	}
}

// Same as D2Oracle_Call but for functions returning a 64-bit value (edx:eax on
// x86). Casting `fn` to a uint64_t-returning pointer makes the COMPILER capture
// edx:eax as the return -- still no hand-asm. Needed by e.g. the RNG family
// (SEED_RollRandomNumber: __fastcall(D2SeedStrc*) -> u64).
#define D2ORACLE_CALL64_CASES(CONV)                                                \
	switch (n)                                                                      \
	{                                                                              \
	case 0: return ((U64(CONV*)())fn)();                                           \
	case 1: return ((U64(CONV*)(U))fn)(a[0]);                                      \
	case 2: return ((U64(CONV*)(U, U))fn)(a[0], a[1]);                             \
	case 3: return ((U64(CONV*)(U, U, U))fn)(a[0], a[1], a[2]);                    \
	case 4: return ((U64(CONV*)(U, U, U, U))fn)(a[0], a[1], a[2], a[3]);           \
	case 5: return ((U64(CONV*)(U, U, U, U, U))fn)(a[0], a[1], a[2], a[3], a[4]);  \
	case 6: return ((U64(CONV*)(U, U, U, U, U, U))fn)(                             \
		a[0], a[1], a[2], a[3], a[4], a[5]);                                       \
	case 7: return ((U64(CONV*)(U, U, U, U, U, U, U))fn)(                          \
		a[0], a[1], a[2], a[3], a[4], a[5], a[6]);                                 \
	case 8: return ((U64(CONV*)(U, U, U, U, U, U, U, U))fn)(                       \
		a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);                           \
	default: return 0;                                                             \
	}

using U64 = uint64_t;

extern "C" uint64_t D2Oracle_Call64(void* fn, int cc, const uint32_t* a, int n)
{
	if (!fn || n < 0 || n > 8)
		return 0;
	switch (cc)
	{
	case D2CC_CDECL:    D2ORACLE_CALL64_CASES(__cdecl)
	case D2CC_STDCALL:  D2ORACLE_CALL64_CASES(__stdcall)
	case D2CC_FASTCALL: D2ORACLE_CALL64_CASES(__fastcall)
	case D2CC_THISCALL: D2ORACLE_CALL64_CASES(__thiscall)
	default: return 0;
	}
}

// Register-explicit call (design detail B, register-ABI path). Some D2 functions
// use non-standard/custom conventions the compiler can't express -- e.g.
// SEED_GetRandomNumber takes its `max` arg in EAX, not the fastcall EDX (found in
// the batch shakeout). This sets ALL 6 GP registers from io[] before the call and
// captures them after, so the caller controls exactly which value lands in which
// register -- matching Ghidra's param_layout register model. REGISTER-ONLY (no
// stack args): the target must not expect stack arguments (true of the RNG
// register family). A hand-asm stub is unavoidable here (no C++ way to pin
// arbitrary values into ESI/EDI/EAX across a call); it is SEH-guarded by the
// server so a bad call can't take down the game.
//
//   io layout (in on entry, OUT on return): [0]=EAX [1]=ECX [2]=EDX [3]=EBX
//                                            [4]=ESI [5]=EDI
extern "C" __declspec(naked) void D2Oracle_CallRegs(void* /*fn*/, uint32_t* /*io*/)
{
	__asm {
		push ebp
		mov  ebp, esp
		push ebx
		push esi
		push edi
		mov  eax, [ebp + 12]      // io ptr
		mov  ecx, [eax + 4]       // ECX
		mov  edx, [eax + 8]       // EDX
		mov  ebx, [eax + 12]      // EBX
		mov  esi, [eax + 16]      // ESI
		mov  edi, [eax + 20]      // EDI
		mov  eax, [eax + 0]       // EAX last (io ptr now lost; reloaded below)
		call dword ptr [ebp + 8]  // fn -- register-only, no stack args
		// capture outputs: push, reload io, pop into fields
		push edi
		push esi
		push ebx
		push edx
		push ecx
		push eax
		mov  eax, [ebp + 12]      // io ptr (eax output already on stack)
		pop  dword ptr [eax + 0]  // io[0] = EAX
		pop  dword ptr [eax + 4]  // io[1] = ECX
		pop  dword ptr [eax + 8]  // io[2] = EDX
		pop  dword ptr [eax + 12] // io[3] = EBX
		pop  dword ptr [eax + 16] // io[4] = ESI
		pop  dword ptr [eax + 20] // io[5] = EDI
		pop  edi
		pop  esi
		pop  ebx
		mov  esp, ebp
		pop  ebp
		ret
	}
}
