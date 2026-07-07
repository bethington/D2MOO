// init_timer_state.cpp -- production loop demo (2026-07-07): a NEW D2Common leaf
// reimplemented from its Ghidra decompilation + disassembly, beyond the original
// shakeout 13. Proven the full loop extends to a fresh function.
//
// InitTimerState (PD2 0x36750, ordinal #10936): void __fastcall(uint* pState).
// Disassembly (asm is ground truth):
//   MOV dword ptr [ECX],   0x1      ; pState[0] = 1   (timer active flag)
//   MOV dword ptr [ECX+4], 0x29a    ; pState[1] = 666 (default duration ticks)
//   RET                              ; fastcall, single reg arg (ECX), no stack cleanup
// Class B (void, 8-byte out-param buffer; output independent of input).
// D2MOO_REIMPL_EXPORT: InitTimerState
extern "C" void __fastcall InitTimerState(unsigned int* pState)
{
	pState[0] = 1;
	pState[1] = 0x29a;
}
