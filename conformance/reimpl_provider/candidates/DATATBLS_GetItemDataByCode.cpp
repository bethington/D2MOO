// D2MOO_REIMPL_EXPORT: DATATBLS_GetItemDataByCode
#include "../provider_runtime.h"

// TRUE ABI (from disasm @0x6fd9e1d0): __stdcall with THREE stack slots (RET 0xC),
// only the first used. Ghidra's single-param prototype is under-declared -- a
// 1-arg spec made the original pop 12 bytes while the oracle pushed 4, corrupting
// the oracle stack (the batch-1 SEH fault). Declare all three.
extern "C" int __stdcall DATATBLS_GetItemDataByCode(short** pItemCode, int unused2, int unused3)
{
    (void)unused2; (void)unused3;
    if (!pItemCode) return 0;      // orig null path: RET with EAX == pItemCode == 0
    return (int)**pItemCode;       // MOVSX word ptr -- sign-extended short
}
