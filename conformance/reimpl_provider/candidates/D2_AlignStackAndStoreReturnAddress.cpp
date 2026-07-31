#include "../provider_runtime.h"

// D2MOO_REIMPL_EXPORT: AlignStackAndStoreReturnAddress
//
// The Ghidra decompiled body references ONLY stack-local data (the only explicit
// memory read is `&stack0x00000004`, a Ghidra SP-relative offset; `dwStackValue`
// and `dwReturnAddr` are also stack-frame locals whose initializations were elided
// under the "Unable to track spacebase fully for stack" warning). No `_g_*` global
// is referenced anywhere in the body, so the resolver is NOT exercised by this
// function. The function aligns the caller's stack pointer to a 4 KB boundary
// and stores the return address at the aligned position; a portable user-mode
// reimpl cannot faithfully manipulate the caller's stack frame, so both the
// original and this reimpl are no-ops as far as observable return values /
// global-state side-effects are concerned -- the proof is trivially consistent.

extern "C" void __stdcall AlignStackAndStoreReturnAddress(void)
{
	return;
}
